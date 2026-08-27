#!/usr/bin/env python3
"""Generates fixed-function pixel shader variants from captured configurations.

Configurations that reduce to the same canonical program share one variant.

Usage:
- generate.py --states states.jsonl --output generated

Inputs:
- --states: JSONL file, one record per observed configuration, holding four
  texture stages, the specular, fog and alpha-test fields, and its recorded
  rapidhash
- --output: directory receiving the generated files

Outputs, under the output directory:
- variant_NNN.hlsl: one wrapper per canonical program, defining FF_* macros
  over FixedFunctionPixelShader.hlsl
- fixed_function_pixel_shader_variants.generated.h: catalog mapping each
  configuration's hash to its full configuration and program key, and each
  program key to its embedded SPIR-V/DXIL blobs; the program table is
  emitted sorted by key so the runtime can binary-search it
- summary.txt: configuration and program counts
- manifest.cmake: variant list plus SHA-256 hashes of the states file, the
  generator, the shared variant_hash.py and every generated file; the build
  uses them to detect stale outputs

PS. Sewer says this is a one-off vibez codo shitto; for a one time export.
    There's harder problems to be focusing on. Banana!

    Like hand optimizing the actual DAMN runtime code.
"""

import argparse
import hashlib
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import variant_hash


STAGE_FIELDS = (
    "color_op",
    "color_arg0",
    "color_arg1",
    "color_arg2",
    "alpha_op",
    "alpha_arg0",
    "alpha_arg1",
    "alpha_arg2",
    "result_arg",
    "texture_sample_type",
)


def normalized_text_bytes(path):
    """File bytes with line endings normalized, so hashes ignore EOL style."""
    return path.read_text(encoding="utf-8").replace("\r\n", "\n").replace("\r", "\n").encode("utf-8")


def write_text(path, value):
    path.write_bytes(value.encode("utf-8"))


def load_states(path):
    """Loads (hash, configuration) pairs from a states JSONL file.

    Raises RuntimeError when a record has an unsupported version or a stage
    count other than four, or when two distinct configurations share one
    hash. Malformed JSON raises json.JSONDecodeError. Recorded hashes are
    the catalog state hashes, computed over the configuration's 45 bytes
    in C++ struct order; the C++ runtime verifies them against its own
    rapidhash in debug builds."""
    states = []
    hashes = {}

    for line_number, line in enumerate(path.read_text().splitlines(), 1):
        if not line:
            continue

        record = json.loads(line)
        if record.get("version") != 1:
            raise RuntimeError(f"{path}:{line_number}: unsupported version")
        if len(record.get("stages", [])) != 4:
            raise RuntimeError(f"{path}:{line_number}: expected four stages")

        values = []
        for stage in record["stages"]:
            values.append(tuple(int(stage[field]) for field in STAGE_FIELDS))

        state = (
            tuple(values),
            int(record["specular_enable"]),
            int(record["fog_enable"]),
            int(record["fog_table_mode"]),
            int(record["alpha_test_enable"]),
            int(record["alpha_func"]),
        )
        state_hash = int(record["hash"], 16)

        if state_hash in hashes and hashes[state_hash] != state:
            raise RuntimeError(f"{path}:{line_number}: hash collision {state_hash:016X}")

        hashes[state_hash] = state
        states.append((state_hash, state))

    return states


def canonical_program(state):
    """Reduces a configuration to the canonical program it compiles into.

    Trailing stages whose color_op is the disable op are dropped, and
    fog_table_mode and alpha_func are zeroed when their feature is off."""
    stages, specular_enable, fog_enable, fog_table_mode, alpha_test_enable, alpha_func = state
    active_stage_count = 0
    # color_op 1 is the disable op: keep stages up to the last active one.
    for stage_index, stage in enumerate(stages):
        if stage[0] != 1:
            active_stage_count = stage_index + 1

    active_stages = stages[:active_stage_count]

    return (
        tuple(active_stages),
        int(specular_enable != 0),
        int(fog_enable != 0),
        fog_table_mode if fog_enable else 0,
        int(alpha_test_enable != 0),
        alpha_func if alpha_test_enable else 0,
    )


def flatten_program(program):
    """Flattens a program into the uint32 words rapidhash64 hashes."""
    stages, specular_enable, fog_enable, fog_table_mode, alpha_test_enable, alpha_func = program
    values = [len(stages)]
    for stage in stages:
        values.extend(stage)
    values.extend((specular_enable, fog_enable, fog_table_mode, alpha_test_enable, alpha_func))
    return values


def complete_stages(program):
    """Pads active stages back to four with a disabled stage."""
    stages = list(program[0])
    disabled = (1, 1, 0, 1, 1, 1, 0, 1, 1, 0)
    return stages + [disabled] * (4 - len(stages))


def write_wrapper(path, program):
    stages = complete_stages(program)
    lines = ["#define FIXED_FUNCTION_PIXEL_SHADER_VARIANT 1"]

    for stage_index, stage in enumerate(stages):
        for field_index, field in enumerate(STAGE_FIELDS):
            lines.append(f"#define FF_{field.upper()}_{stage_index}_VALUE {stage[field_index]}")

    lines.extend((
        f"#define FF_ACTIVE_STAGE_COUNT_VALUE {len(program[0])}",
        f"#define FF_SPECULAR_ENABLE_VALUE {program[1]}",
        f"#define FF_FOG_ENABLE_VALUE {program[2]}",
        f"#define FF_FOG_TABLE_MODE_VALUE {program[3]}",
        f"#define FF_ALPHA_TEST_ENABLE_VALUE {program[4]}",
        f"#define FF_ALPHA_FUNC_VALUE {program[5]}",
        '#include "FixedFunctionPixelShader.hlsl"',
        "",
    ))
    write_text(path, "\n".join(lines))


def cpp_array(values):
    return "{" + ",".join(str(value) for value in values) + "}"


def cpp_state(state):
    """Renders a configuration as a C++ initializer, one array per stage field."""
    stages, specular_enable, fog_enable, fog_table_mode, alpha_test_enable, alpha_func = state
    field_arrays = []
    for field_index in range(len(STAGE_FIELDS)):
        field_arrays.append(cpp_array(stage[field_index] for stage in stages))
    return "{" + ",".join(field_arrays + [
        str(specular_enable),
        str(fog_enable),
        str(fog_table_mode),
        str(alpha_test_enable),
        str(alpha_func),
    ]) + "}"


def write_catalog(path, states, programs, program_indices, program_keys):
    lines = ["#pragma once", ""]

    for index in range(len(programs)):
        lines.append(f'#include "variant_{index:03}.hlsl.spirv.h"')

    lines.extend(("", "#ifdef SHADOW_RECOMP_D3D12"))
    for index in range(len(programs)):
        lines.append(f'#include "variant_{index:03}.hlsl.dxil.h"')
    lines.extend(("#endif", "", "namespace FixedFunctionPixelShaderVariants::Generated", "{"))
    lines.extend((
        "struct Program",
        "{",
        "    uint64_t key;",
        "    const uint8_t* spirv;",
        "    size_t spirvSize;",
        "#ifdef SHADOW_RECOMP_D3D12",
        "    const uint8_t* dxil;",
        "    size_t dxilSize;",
        "#endif",
        "};",
        "",
        "static const Program Programs[] =",
        "{",
    ))

    # Rows are emitted in key order so the runtime binary-searches the
    # table; blob names still follow the stable variant numbering.
    for program, program_key in sorted(program_keys.items(), key=lambda item: item[1]):
        index = program_indices[program]
        lines.extend((
            "    {",
            f"        0x{program_key:016X}ULL,",
            f"        g_variant_{index:03}_spirv, sizeof(g_variant_{index:03}_spirv),",
            "#ifdef SHADOW_RECOMP_D3D12",
            f"        g_variant_{index:03}_dxil, sizeof(g_variant_{index:03}_dxil),",
            "#endif",
            "    },",
        ))

    lines.extend((
        "};",
        "",
        "struct State",
        "{",
        "    uint64_t hash;",
        "    FixedFunctionPixelShaderState state;",
        "    uint64_t programKey;",
        "};",
        "",
        "static const State States[] =",
        "{",
    ))

    for state_hash, state in sorted(states):
        program = canonical_program(state)
        lines.append(
            f"    {{ 0x{state_hash:016X}ULL, {cpp_state(state)}, "
            f"0x{program_keys[program]:016X}ULL }},"
        )

    lines.extend(("};", "}" , ""))
    lines.insert(-2, f"static_assert(std::size(Programs) == {len(programs)});")
    lines.insert(-2, f"static_assert(std::size(States) == {len(states)});")
    write_text(path, "\n".join(lines))


def write_manifest(path, states_path, generator_path, variant_hash_path, output_path, program_count):
    state_hash = hashlib.sha256(normalized_text_bytes(states_path)).hexdigest()
    generator_hash = hashlib.sha256(normalized_text_bytes(generator_path)).hexdigest()
    variant_hash_hash = hashlib.sha256(normalized_text_bytes(variant_hash_path)).hexdigest()
    lines = [
        f'set(FIXED_FUNCTION_PIXEL_SHADER_STATES_SHA256 "{state_hash}")',
        f'set(FIXED_FUNCTION_PIXEL_SHADER_GENERATOR_SHA256 "{generator_hash}")',
        f'set(FIXED_FUNCTION_PIXEL_SHADER_VARIANT_HASH_SHA256 "{variant_hash_hash}")',
        f'set(FIXED_FUNCTION_PIXEL_SHADER_VARIANT_COUNT {program_count})',
        'set(FIXED_FUNCTION_PIXEL_SHADER_VARIANTS',
    ]
    for index in range(program_count):
        lines.append(f'    "variant_{index:03}"')
    lines.extend((")", "", "set(FIXED_FUNCTION_PIXEL_SHADER_GENERATED_FILES"))
    generated_files = ["fixed_function_pixel_shader_variants.generated.h", "summary.txt"]
    generated_files.extend(f"variant_{index:03}.hlsl" for index in range(program_count))
    for file_name in generated_files:
        lines.append(f'    "{file_name}"')
    lines.extend((")", "", "set(FIXED_FUNCTION_PIXEL_SHADER_GENERATED_SHA256"))
    for file_name in generated_files:
        file_hash = hashlib.sha256(normalized_text_bytes(output_path / file_name)).hexdigest()
        lines.append(f'    "{file_hash}"')
    lines.extend((")", ""))
    write_text(path, "\n".join(lines))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--states", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()

    # Identical records collapse; load_states rejects hashes shared by
    # distinct states, so the emitted table holds each hash exactly once.
    states = sorted(set(load_states(args.states)))
    # Sorted so variant numbering stays stable across regenerations.
    programs = sorted({canonical_program(state) for _, state in states})
    program_indices = {program: index for index, program in enumerate(programs)}
    program_keys = {program: variant_hash.rapidhash64(flatten_program(program)) for program in programs}

    if len(set(program_keys.values())) != len(programs):
        raise RuntimeError("program key collision")

    args.output.mkdir(parents=True, exist_ok=True)
    # Drop stale wrappers so a shrinking variant set leaves no orphans.
    for wrapper in args.output.glob("variant_*.hlsl"):
        wrapper.unlink()

    for program, index in program_indices.items():
        write_wrapper(args.output / f"variant_{index:03}.hlsl", program)

    write_catalog(
        args.output / "fixed_function_pixel_shader_variants.generated.h",
        states,
        programs,
        program_indices,
        program_keys,
    )

    write_text(args.output / "summary.txt", f"states={len(states)}\nprograms={len(programs)}\n")

    write_manifest(
        args.output / "manifest.cmake",
        args.states,
        Path(__file__),
        Path(variant_hash.__file__),
        args.output,
        len(programs),
    )


if __name__ == "__main__":
    main()
