#!/usr/bin/env python3
"""Generates fixed-function vertex shader variants from captured configurations.

Captured configurations are normalized into effective configurations, and
configurations that reduce to the same canonical program share one variant.

Usage:
- generate.py --states states.jsonl --output generated

Inputs:
- --states: JSONL file, one record per captured configuration, holding the
  scalar, array and tail fields plus its recorded rapidhash
- --output: directory receiving the generated files

Outputs, under the output directory:
- variant_NNN.hlsl: one wrapper per canonical program, defining FFVS_* macros
  over FixedFunctionVertexShader.hlsl
- fixed_function_vertex_shader_variants.generated.h: catalog mapping each
  effective configuration's key to its full configuration and program key,
  and each program key to its embedded SPIR-V/DXIL blobs; the program table
  is emitted sorted by key so the runtime can binary-search it
- summary.txt: captured and effective configuration and program counts
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


SCALAR_FIELDS = (
    "vertex_blend_matrix_count",
    "vertex_blend_calc_last_weight",
    "lighting_enable",
    "two_sided_lighting_enable",
    "local_viewer_enable",
    "normalize_normals_enable",
    "ambient_material_source",
    "diffuse_material_source",
    "specular_material_source",
    "emissive_material_source",
    "fog_enable",
    "fog_depth_mode",
)

ARRAY_FIELDS = (
    ("light_types", 8),
    ("texture_transform_counts", 4),
    ("texture_transform_projected", 4),
    ("texcoord_indices", 4),
    ("texcoord_generation", 4),
    ("texcoord_component_counts", 4),
)

TAIL_FIELDS = (
    "vertex_register_default_mask",
    "point_sprite_enable",
    "point_scale_enable",
    "specular_enable",
)


def normalized_text_bytes(path):
    """File bytes with line endings normalized, so hashes ignore EOL style."""
    return path.read_text(encoding="utf-8").replace("\r\n", "\n").replace("\r", "\n").encode("utf-8")


def write_text(path, value):
    path.write_bytes(value.encode("utf-8"))


def load_states(path):
    """Loads (hash, configuration) pairs from a states JSONL file.

    Raises RuntimeError when a record has an unsupported version, an array
    field with the wrong length, or when two distinct configurations share
    one hash. Malformed JSON raises json.JSONDecodeError. Recorded hashes
    are the catalog state hashes; the C++ runtime verifies them against its
    own rapidhash in debug builds."""
    states = []
    hashes = {}

    for line_number, line in enumerate(path.read_text().splitlines(), 1):
        if not line:
            continue

        record = json.loads(line)
        if record.get("version") != 1:
            raise RuntimeError(f"{path}:{line_number}: unsupported version")

        scalars = tuple(int(record[field]) for field in SCALAR_FIELDS)
        arrays = []
        for field, count in ARRAY_FIELDS:
            values = tuple(int(value) for value in record[field])
            if len(values) != count:
                raise RuntimeError(f"{path}:{line_number}: {field} must contain {count} values")
            arrays.append(values)
        tail = tuple(int(record[field]) for field in TAIL_FIELDS)
        state = scalars + tuple(arrays) + tail
        state_hash = int(record["hash"], 16)

        if state_hash in hashes and hashes[state_hash] != state:
            raise RuntimeError(f"{path}:{line_number}: hash collision {state_hash:016X}")

        hashes[state_hash] = state
        states.append((state_hash, state))

    return states


def normalize_state(state):
    """Canonicalizes a captured configuration into the effective configuration.

    Fields that cannot affect rendering are cleared: lighting off removes
    light types, material sources and specular; local viewer needs at least
    one light; fog off clears the depth mode; point scale needs point
    sprites. Vertex register defaults are kept only for registers the
    configuration can read."""
    (
        matrix_count, calc_last_weight, lighting_enable, two_sided_lighting_enable,
        local_viewer_enable, normalize_normals_enable, ambient_material_source,
        diffuse_material_source, specular_material_source, emissive_material_source,
        fog_enable, fog_depth_mode, light_types, texture_transform_counts,
        _texture_transform_projected, texcoord_indices, texcoord_generation,
        texcoord_component_counts, vertex_register_default_mask,
        point_sprite_enable, point_scale_enable,
        specular_enable,
    ) = state

    lighting_enable = int(lighting_enable != 0)
    calc_last_weight = int(calc_last_weight != 0)
    normalize_normals_enable = int(normalize_normals_enable != 0)
    if lighting_enable:
        light_types = tuple(light_types)
        two_sided_lighting_enable = int(two_sided_lighting_enable != 0)
        local_viewer_enable = int(local_viewer_enable != 0 and any(light_types))
        material_sources = (
            ambient_material_source,
            diffuse_material_source,
            specular_material_source,
            emissive_material_source,
        )
    else:
        light_types = (0,) * 8
        two_sided_lighting_enable = 0
        local_viewer_enable = 0
        material_sources = (0, 0, 0, 0)

    texture_states = []
    normalized_transform_counts = []
    normalized_texcoord_indices = []
    used_component_counts = set()
    normal_used = bool(lighting_enable)
    for stage in range(4):
        # Generation 0 passes an existing texcoord through; other generations
        # synthesize coordinates, so the index and component count go unused.
        generation = texcoord_generation[stage]
        if generation == 0:
            index = texcoord_indices[stage]
            component_count = texcoord_component_counts[index] if index < 4 else 0
            if index < 4:
                used_component_counts.add(index)
        else:
            index = 0
            component_count = 0

        if generation in (1, 3, 5):
            # Texcoord generations that read the vertex normal.
            normal_used = True

        texture_states.append((
            int(texture_transform_counts[stage] != 0),
            index,
            generation,
            component_count,
        ))
        normalized_transform_counts.append(int(texture_transform_counts[stage] != 0))
        normalized_texcoord_indices.append(index)

    fog_enable = int(fog_enable != 0)
    point_sprite_enable = int(point_sprite_enable != 0)
    specular_enable = int(specular_enable != 0 and lighting_enable)

    normalized_component_counts = tuple(
        texcoord_component_counts[index] if index in used_component_counts else 0
        for index in range(4)
    )

    # Input registers this configuration can read (register 0 is position);
    # defaults are kept below only for these.
    used_registers = {0}
    if matrix_count > 1 or not calc_last_weight:
        used_registers.add(1)
    if normal_used:
        used_registers.add(2)
    if not lighting_enable or 1 in material_sources:
        used_registers.add(3)
    if not lighting_enable or 2 in material_sources or (fog_enable and fog_depth_mode == 0):
        used_registers.add(4)
    if not (lighting_enable and two_sided_lighting_enable) or 1 in material_sources:
        used_registers.add(7)
    if not (lighting_enable and two_sided_lighting_enable) or 2 in material_sources:
        used_registers.add(8)
    for stage in range(4):
        if texcoord_generation[stage] == 0 and normalized_texcoord_indices[stage] < 4:
            used_registers.add(9 + normalized_texcoord_indices[stage])
    used_register_mask = sum(1 << index for index in used_registers)

    normalized_state = (
        matrix_count,
        calc_last_weight,
        lighting_enable,
        two_sided_lighting_enable,
        local_viewer_enable,
        int(normalize_normals_enable != 0 and normal_used),
        material_sources[0],
        material_sources[1],
        material_sources[2],
        material_sources[3],
        fog_enable,
        fog_depth_mode if fog_enable else 0,
        light_types,
        tuple(normalized_transform_counts),
        (0,) * 4,
        tuple(normalized_texcoord_indices),
        tuple(texcoord_generation),
        normalized_component_counts,
        vertex_register_default_mask & used_register_mask,
        point_sprite_enable,
        int(point_scale_enable != 0 and point_sprite_enable),
        specular_enable,
    )

    return normalized_state


def canonical_program(state):
    """Reduces a configuration, re-normalized first, to the canonical program.

    Each stage contributes one texture state tuple; fog depth mode is zeroed
    when fog is off, point scale requires point sprites, and specular
    requires lighting."""
    state = normalize_state(state)
    (
        matrix_count, calc_last_weight, lighting_enable, two_sided_lighting_enable,
        local_viewer_enable, normalize_normals_enable, ambient_material_source,
        diffuse_material_source, specular_material_source, emissive_material_source,
        fog_enable, fog_depth_mode, light_types, texture_transform_counts,
        _texture_transform_projected, texcoord_indices, texcoord_generation,
        texcoord_component_counts, vertex_register_default_mask,
        point_sprite_enable, point_scale_enable,
        specular_enable,
    ) = state

    material_sources = (
        ambient_material_source,
        diffuse_material_source,
        specular_material_source,
        emissive_material_source,
    )
    texture_states = tuple(
        (
            texture_transform_counts[stage],
            texcoord_indices[stage],
            texcoord_generation[stage],
            texcoord_component_counts[texcoord_indices[stage]]
                if texcoord_generation[stage] == 0 and texcoord_indices[stage] < 4 else 0,
        )
        for stage in range(4)
    )

    return (
        matrix_count,
        int(calc_last_weight != 0),
        lighting_enable,
        two_sided_lighting_enable,
        local_viewer_enable,
        normalize_normals_enable,
        material_sources,
        fog_enable,
        fog_depth_mode if fog_enable else 0,
        light_types,
        texture_states,
        vertex_register_default_mask,
        point_sprite_enable,
        int(point_scale_enable != 0 and point_sprite_enable),
        specular_enable,
    )


def flatten_program(program):
    """Flattens a program into the uint32 words rapidhash64 hashes."""
    values = list(program[:6])
    values.extend(program[6])
    values.extend(program[7:9])
    values.extend(program[9])
    for texture_state in program[10]:
        values.extend(texture_state)
    values.extend(program[11:])
    return values


def write_wrapper(path, program):
    """Writes variant_NNN.hlsl, the FFVS_* macro block over the shader body."""
    (
        matrix_count, calc_last_weight, lighting_enable, two_sided_lighting_enable,
        local_viewer_enable, normalize_normals_enable, material_sources,
        fog_enable, fog_depth_mode, light_types, texture_states,
        vertex_register_default_mask, point_sprite_enable, point_scale_enable,
        specular_enable,
    ) = program

    lines = [
        "#define FIXED_FUNCTION_VERTEX_SHADER_VARIANT 1",
        f"#define FFVS_VERTEX_BLEND_MATRIX_COUNT_VALUE {matrix_count}",
        f"#define FFVS_VERTEX_BLEND_CALC_LAST_WEIGHT_VALUE {calc_last_weight}",
        f"#define FFVS_LIGHTING_ENABLE_VALUE {lighting_enable}",
        f"#define FFVS_TWO_SIDED_LIGHTING_ENABLE_VALUE {two_sided_lighting_enable}",
        f"#define FFVS_LOCAL_VIEWER_ENABLE_VALUE {local_viewer_enable}",
        f"#define FFVS_NORMALIZE_NORMALS_ENABLE_VALUE {normalize_normals_enable}",
        f"#define FFVS_AMBIENT_MATERIAL_SOURCE_VALUE {material_sources[0]}",
        f"#define FFVS_DIFFUSE_MATERIAL_SOURCE_VALUE {material_sources[1]}",
        f"#define FFVS_SPECULAR_MATERIAL_SOURCE_VALUE {material_sources[2]}",
        f"#define FFVS_EMISSIVE_MATERIAL_SOURCE_VALUE {material_sources[3]}",
        f"#define FFVS_FOG_ENABLE_VALUE {fog_enable}",
        f"#define FFVS_FOG_DEPTH_MODE_VALUE {fog_depth_mode}",
        f"#define FFVS_VERTEX_REGISTER_DEFAULT_MASK_VALUE {vertex_register_default_mask}",
        f"#define FFVS_POINT_SPRITE_ENABLE_VALUE {point_sprite_enable}",
        f"#define FFVS_POINT_SCALE_ENABLE_VALUE {point_scale_enable}",
        f"#define FFVS_SPECULAR_ENABLE_VALUE {specular_enable}",
    ]

    for index, light_type in enumerate(light_types):
        lines.append(f"#define FFVS_LIGHT_TYPE_{index}_VALUE {light_type}")
    for index, texture_state in enumerate(texture_states):
        lines.extend((
            f"#define FFVS_TEXTURE_TRANSFORM_ENABLE_{index}_VALUE {texture_state[0]}",
            f"#define FFVS_TEXCOORD_INDEX_{index}_VALUE {texture_state[1]}",
            f"#define FFVS_TEXCOORD_GENERATION_{index}_VALUE {texture_state[2]}",
            f"#define FFVS_TEXCOORD_COMPONENT_COUNT_{index}_VALUE {texture_state[3]}",
        ))

    lines.extend(('#include "FixedFunctionVertexShader.hlsl"', ""))
    write_text(path, "\n".join(lines))


def cpp_array(values):
    return "{" + ",".join(str(value) for value in values) + "}"


def cpp_state(state):
    """Renders a configuration as a C++ initializer, scalars, then arrays, then tail fields.

    The texture_transform_projected entry (state[14]) has no struct field, so
    it is skipped."""
    scalars = [str(value) for value in state[:12]]
    arrays = [cpp_array(state[index]) for index in (12, 13, 15, 16, 17)]
    tail = [str(value) for value in state[18:]]
    return "{" + ",".join(scalars + arrays + tail) + "}"


def write_catalog(path, states, programs, program_indices, program_keys):
    """Writes the C++ catalog: program table keyed by rapidhash key, state
    table mapping each effective configuration's key to its program key."""
    lines = ["#pragma once", ""]
    for index in range(len(programs)):
        lines.append(f'#include "variant_{index:03}.hlsl.spirv.h"')

    lines.extend(("", "#ifdef SHADOW_RECOMP_D3D12"))
    for index in range(len(programs)):
        lines.append(f'#include "variant_{index:03}.hlsl.dxil.h"')
    lines.extend(("#endif", "", "namespace FixedFunctionVertexShaderVariants::Generated", "{"))
    lines.extend((
        "struct Program", "{", "    uint64_t key;", "    const uint8_t* spirv;", "    size_t spirvSize;",
        "#ifdef SHADOW_RECOMP_D3D12", "    const uint8_t* dxil;", "    size_t dxilSize;", "#endif", "};", "",
        "static const Program Programs[] =", "{",
    ))

    # Rows are emitted in key order so the runtime binary-searches the
    # table; blob names still follow the stable variant numbering.
    for program, program_key in sorted(program_keys.items(), key=lambda item: item[1]):
        index = program_indices[program]
        lines.extend((
            "    {", f"        0x{program_key:016X}ULL,",
            f"        g_variant_{index:03}_spirv, sizeof(g_variant_{index:03}_spirv),",
            "#ifdef SHADOW_RECOMP_D3D12",
            f"        g_variant_{index:03}_dxil, sizeof(g_variant_{index:03}_dxil),",
            "#endif", "    },",
        ))

    lines.extend((
        "};", "", "struct State", "{", "    uint64_t hash;",
        "    FixedFunctionVertexShaderControlState state;", "    uint64_t programKey;", "};", "",
        "static const State States[] =", "{",
    ))

    for state_hash, state in sorted(states):
        program = canonical_program(state)
        lines.append(f"    {{ 0x{state_hash:016X}ULL, {cpp_state(state)}, 0x{program_keys[program]:016X}ULL }},")

    lines.extend(("};", f"static_assert(std::size(Programs) == {len(programs)});",
        f"static_assert(std::size(States) == {len(states)});", "}", ""))
    write_text(path, "\n".join(lines))


def write_manifest(path, states_path, generator_path, variant_hash_path, output_path, program_count):
    """Writes the CMake manifest with input and output hashes for staleness checks."""
    state_hash = hashlib.sha256(normalized_text_bytes(states_path)).hexdigest()
    generator_hash = hashlib.sha256(normalized_text_bytes(generator_path)).hexdigest()
    variant_hash_hash = hashlib.sha256(normalized_text_bytes(variant_hash_path)).hexdigest()
    lines = [
        f'set(FIXED_FUNCTION_VERTEX_SHADER_STATES_SHA256 "{state_hash}")',
        f'set(FIXED_FUNCTION_VERTEX_SHADER_GENERATOR_SHA256 "{generator_hash}")',
        f'set(FIXED_FUNCTION_VERTEX_SHADER_VARIANT_HASH_SHA256 "{variant_hash_hash}")',
        f'set(FIXED_FUNCTION_VERTEX_SHADER_VARIANT_COUNT {program_count})',
        'set(FIXED_FUNCTION_VERTEX_SHADER_VARIANTS',
    ]
    for index in range(program_count):
        lines.append(f'    "variant_{index:03}"')
    lines.extend((")", "", "set(FIXED_FUNCTION_VERTEX_SHADER_GENERATED_FILES"))
    generated_files = ["fixed_function_vertex_shader_variants.generated.h", "summary.txt"]
    generated_files.extend(f"variant_{index:03}.hlsl" for index in range(program_count))
    for file_name in generated_files:
        lines.append(f'    "{file_name}"')
    lines.extend((")", "", "set(FIXED_FUNCTION_VERTEX_SHADER_GENERATED_SHA256"))
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

    observed_states = load_states(args.states)
    # Distinct captures can normalize to the same effective configuration;
    # the corpus stores effective states, so their recorded hashes carry
    # over unchanged.
    states = sorted({(entry_hash, normalize_state(state)) for entry_hash, state in observed_states})
    if len({entry_hash for entry_hash, _ in states}) != len(states):
        raise RuntimeError("state hash collision")

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
        args.output / "fixed_function_vertex_shader_variants.generated.h",
        states, programs, program_indices, program_keys,
    )
    write_text(args.output / "summary.txt",
        f"observed_states={len(observed_states)}\neffective_states={len(states)}\nprograms={len(programs)}\n")
    write_manifest(
        args.output / "manifest.cmake", args.states, Path(__file__), Path(variant_hash.__file__),
        args.output, len(programs),
    )


if __name__ == "__main__":
    main()
