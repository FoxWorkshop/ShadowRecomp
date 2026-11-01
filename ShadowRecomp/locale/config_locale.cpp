#include <user/config.h>

/*
    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!! LOCALISATION NOTES !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    - Ensure brand names are always presented on the same line.

      Correct:
      This is a string that contains a brand name like
      Xbox, which is one of the few consoles to have a port of
      Shadow the Hedgehog.

      Incorrect:
      This is a string that contains a brand name like Xbox
      which is one of the few consoles to have a port of Shadow
      Hedgehog.

    - Ensure your locale is added in the correct order following the language enum.

      Correct:
      {
          { ELanguage::English,  "Example" },
          { ELanguage::Japanese, "Example" },
          { ELanguage::German,   "Example" },
          { ELanguage::French,   "Example" },
          { ELanguage::Spanish,  "Example" },
          { ELanguage::Italian,  "Example" }
      }

      Incorrect:
      {
          { ELanguage::English,  "Example" },
          { ELanguage::French,   "Example" },
          { ELanguage::Spanish,  "Example" },
          { ELanguage::German,   "Example" },
          { ELanguage::Italian,  "Example" },
          { ELanguage::Japanese, "Example" }
      }

    - Japanese localization should use furigana for message windows, options items,
    options descriptions and installer text.

      Furigana should be included in the text as such:
      "[私:わたし]はジョージです。"

    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*/

#define CONFIG_DEFINE_LOCALE(name) \
    CONFIG_LOCALE g_##name##_locale =

#define CONFIG_DEFINE_ENUM_LOCALE(type) \
    CONFIG_ENUM_LOCALE(type) g_##type##_locale =

// TODO: Retranslate, I dropped "and logos" in EN
CONFIG_DEFINE_LOCALE(Language)
{
    { ELanguage::English, { "Language", "Change the language used for text." } },
    { ELanguage::Japanese, { "[言語:げんご]", "ゲーム[内:ない]の\u200B[表示言語:ひょうじげんご]を\u200B[変更:へんこう]できます" } },
    { ELanguage::German,   { "Sprache", "Ändere die Sprache für Text und Logos." } },
    { ELanguage::French,   { "Langue", "Modifie la langue utilisée pour le texte et logos." } },
    { ELanguage::Spanish,  { "Idioma", "Cambia el idioma utilizado para el texto y los logos." } },
    { ELanguage::Italian,  { "Lingua", "Cambia la lingua utilizzata per i testi e i logo." } }
};

// Notes: do not localise this.
CONFIG_DEFINE_ENUM_LOCALE(ELanguage)
{
    {
        ELanguage::English,
        {
            { ELanguage::English,  { "ENGLISH", "" } },
            { ELanguage::Japanese, { "日本語", "" } },
            { ELanguage::German,   { "DEUTSCH", "" } },
            { ELanguage::French,   { "FRANÇAIS", "" } },
            { ELanguage::Spanish,  { "ESPAÑOL", "" } },
            { ELanguage::Italian,  { "ITALIANO", "" } }
        }
    }
};

CONFIG_DEFINE_ENUM_LOCALE(EVoiceLanguage)
{
    {
        ELanguage::English,
        {
            { EVoiceLanguage::English,  { "ENGLISH", "" } },
            { EVoiceLanguage::Japanese, { "JAPANESE", "" } }
        }
    },
    {
        ELanguage::Japanese,
        {
            { EVoiceLanguage::English,  { "英語", "" } },
            { EVoiceLanguage::Japanese, { "日本語", "" } }
        }
    },
    {
        ELanguage::German,
        {
            { EVoiceLanguage::English,  { "ENGLISCH", "" } },
            { EVoiceLanguage::Japanese, { "JAPANISCH", "" } }
        }
    },
    {
        ELanguage::French,
        {
            { EVoiceLanguage::English,  { "ANGLAIS", "" } },
            { EVoiceLanguage::Japanese, { "JAPONAIS", "" } }
        }
    },
    {
        ELanguage::Spanish,
        {
            { EVoiceLanguage::English,  { "INGLÉS", "" } },
            { EVoiceLanguage::Japanese, { "JAPONÉS", "" } }
        }
    },
    {
        ELanguage::Italian,
        {
            { EVoiceLanguage::English,  { "INGLESE", "" } },
            { EVoiceLanguage::Japanese, { "GIAPPONESE", "" } }
        }
    },
};

CONFIG_DEFINE_LOCALE(Hints)
{
    { ELanguage::English,  { "Hints", "Show hints during gameplay." } },
    { ELanguage::Japanese, { "ヒントリング", "ゲーム[内:ない]に\u200Bヒントリングを\u200B[表示:ひょうじ]するか\u200B[選択:せんたく]できます" } },
    { ELanguage::German,   { "Hinweise", "Zeige Hinweise während des Spiels." } },
    { ELanguage::French,   { "Indices", "Affiche les indices pendant le jeu." } },
    { ELanguage::Spanish,  { "Pistas", "Muestra pistas durante el juego." } },
    { ELanguage::Italian,  { "Indizi", "Mostra degli indizzi durante il gioco." } }
};

CONFIG_DEFINE_LOCALE(AchievementNotifications)
{
    { ELanguage::English,  { "Achievement Notifications", "Show notifications for unlocking achievements. Achievements will still be rewarded with notifications disabled." } },
    { ELanguage::Japanese, { "[実績通知:じっせきつうち]", "[実績通知:じっせきつうち]の\u200B[有無:うむ]を\u200B[選択:せんたく]できます オフにしても\u200B[実績:じっせき]は\u200B[付与:ふよ]されます" } },
    { ELanguage::German,   { "Erfolgsbenachrichtigungen", "Zeige Benachrichtigungen für das Freischalten von Erfolgen. Erfolge werden weiterhin freigeschaltet, auch wenn die Benachrichtigungen ausgeschaltet sind." } },
    { ELanguage::French,   { "Notification des succès", "Affiche les notifications pour le déverrouillage des succès. Les succès seront toujours obtenus même si les notifications sont désactivées." } },
    { ELanguage::Spanish,  { "Notificaciones de logros", "Muestra notificaciones al desbloquear logros. Los logros se seguirán obteniendo aunque las notificaciones estén desactivadas." } },
    { ELanguage::Italian,  { "Notifiche obiettivi", "Mostra delle notifiche quando sblocchi degli obiettivi. Gli obiettivi verranno comunque assegnati anche con le notifiche disattivate." } }
};

CONFIG_DEFINE_LOCALE(ConfirmationPrompt)
{
    { ELanguage::English,  { "Confirmation Prompts", "Shows confirmation prompts when loading a save file, exiting to the menu, restarting, and in other similar situations. Disabling this option replicates the behavior of the GameCube version." } },
    { ELanguage::Japanese, { "Confirmation Prompts", "Shows confirmation prompts when loading a save file, exiting to the menu, restarting, and in other similar situations. Disabling this option replicates the behavior of the GameCube version." } },
    { ELanguage::German,   { "Confirmation Prompts", "Shows confirmation prompts when loading a save file, exiting to the menu, restarting, and in other similar situations. Disabling this option replicates the behavior of the GameCube version." } },
    { ELanguage::French,   { "Confirmation Prompts", "Affiche les messages de confirmation lors du chargement d'une sauvegarde, du retour au menu, du redémarrage et dans d'autres situations similaires. Désactiver cette option reproduit le comportement de la version GameCube." } },
    { ELanguage::Spanish,  { "Confirmation Prompts", "Shows confirmation prompts when loading a save file, exiting to the menu, restarting, and in other similar situations. Disabling this option replicates the behavior of the GameCube version." } },
    { ELanguage::Italian,  { "Confirmation Prompts", "Shows confirmation prompts when loading a save file, exiting to the menu, restarting, and in other similar situations. Disabling this option replicates the behavior of the GameCube version." } },
};

CONFIG_DEFINE_LOCALE(ControllerIcons)
{
    { ELanguage::English,  { "Controller Icons", "Change the icons to match your controller." } },
    { ELanguage::Japanese, { "コントローラーアイコン", "ゲーム[内:ない]の\u200Bコントローラーアイコン\u200Bを[変更:へんこう]できます" } },
    { ELanguage::German,   { "Controllersymbole", "Ändere die Controllersymbole, um sie auf dein Modell anzupassen." } },
    { ELanguage::French,   { "Icône des boutons", "Modifie les icônes pour les faire correspondre à votre manette." } },
    { ELanguage::Spanish,  { "Iconos del mando", "Cambia los iconos para que coincidan con tu mando." } },
    { ELanguage::Italian,  { "Icone dei tasti", "Modifica le icone per farle corrispondere con il tuo controller." } }
};

// Japanese Notes: This localization should include furigana in its description.
CONFIG_DEFINE_ENUM_LOCALE(EControllerIcons)
{
    {
        ELanguage::English,
        {
            { EControllerIcons::Auto,        { "AUTO", "Auto: the game will determine which icons to use based on the current input device." } },
            { EControllerIcons::Xbox,        { "XBOX", "" } },
            { EControllerIcons::PlayStation2, { "PLAYSTATION2", "" } },
            { EControllerIcons::Gamecube, { "GAMECUBE", "" } }
        }
    },
    {
        ELanguage::Japanese,
        {
            { EControllerIcons::Auto,        { "自動検出", "[\u2005自動検出\u2005:じどうけんしゅつ]: コントローラーアイコン\u200Bを[使用:しよう]している\u200B[\u2005入\u2005力\u2005:にゅうりょく]デバイスに\u200B[応:おう]じて\u200B[自動的:じどうてき]に\u200B[決定:けってい]します" } },
            { EControllerIcons::Xbox,        { "XBOX", "" } },
            { EControllerIcons::PlayStation2, { "PLAYSTATION2", "" } },
            { EControllerIcons::Gamecube, { "GAMECUBE", "" } }
        }
    },
    {
        ELanguage::German,
        {
            { EControllerIcons::Auto,        { "AUTO", "Auto: Das Spiel erkennt automatisch deinen Controller um die Symbole dementsprechend anzupassen." } },
            { EControllerIcons::Xbox,        { "XBOX", "" } },
            { EControllerIcons::PlayStation2, { "PLAYSTATION2", "" } },
            { EControllerIcons::Gamecube, { "GAMECUBE", "" } }
        }
    },
    {
        ELanguage::French,
        {
            { EControllerIcons::Auto,        { "AUTO", "Auto : le jeu déterminera automatiquement quelles icônes utiliser en fonction du périphérique d'entrée." } },
            { EControllerIcons::Xbox,        { "XBOX", "" } },
            { EControllerIcons::PlayStation2, { "PLAYSTATION2", "" } },
            { EControllerIcons::Gamecube, { "GAMECUBE", "" } }
        }
    },
    {
        ELanguage::Spanish,
        {
            { EControllerIcons::Auto,        { "AUTO", "Auto: el juego determinará de forma automática qué iconos utilizar dependiendo del dispositivo de entrada actual." } },
            { EControllerIcons::Xbox,        { "XBOX", "" } },
            { EControllerIcons::PlayStation2, { "PLAYSTATION2", "" } },
            { EControllerIcons::Gamecube, { "GAMECUBE", "" } }
        }
    },
    {
        ELanguage::Italian,
        {
            { EControllerIcons::Auto,        { "AUTO", "Auto: il gioco determinerà quali icone da utilizzare in base al dispositivo di input attuale." } },
            { EControllerIcons::Xbox,        { "XBOX", "" } },
            { EControllerIcons::PlayStation2, { "PLAYSTATION2", "" } },
            { EControllerIcons::Gamecube, { "GAMECUBE", "" } }
        }
    }
};

CONFIG_DEFINE_LOCALE(HorizontalCamera1P)
{
    { ELanguage::English, { "1P Horizontal Camera", "Change how the camera moves left and right." } },
    { ELanguage::Japanese, { "1Pカメラの[左右:さゆう]", "カメラ[左右:さゆう]の\u200B[動:うご]く[方向:ほうこう]を\u200B[選択:せんたく]できます" } },
    { ELanguage::German,   { "1P Horizontale Kamera", "Ändere wie sich die Kamera nach links und rechts bewegt." } },
    { ELanguage::French,   { "1P Caméra horizontale", "Modifie la rotation horizontale de la caméra." } },
    { ELanguage::Spanish,  { "1P Cámara horizontal", "Cambia cómo se mueve la camara hacia la izquierda y la derecha." } },
    { ELanguage::Italian,  { "1P Telecamera orizzontale", "Modifica come la telecamera si muove da sinistra a destra." } }
};

CONFIG_DEFINE_LOCALE(HorizontalCamera2P)
{
    { ELanguage::English, { "2P Horizontal Camera", "Change how the camera moves left and right." } },
    { ELanguage::Japanese, { "2Pカメラの[左右:さゆう]", "カメラ[左右:さゆう]の\u200B[動:うご]く[方向:ほうこう]を\u200B[選択:せんたく]できます" } },
    { ELanguage::German,   { "2P Horizontale Kamera", "Ändere wie sich die Kamera nach links und rechts bewegt." } },
    { ELanguage::French,   { "2P Caméra horizontale", "Modifie la rotation horizontale de la caméra." } },
    { ELanguage::Spanish,  { "2P Cámara horizontal", "Cambia cómo se mueve la camara hacia la izquierda y la derecha." } },
    { ELanguage::Italian,  { "2P Telecamera orizzontale", "Modifica come la telecamera si muove da sinistra a destra." } }
};

CONFIG_DEFINE_LOCALE(HorizontalCamera3P)
{
    { ELanguage::English, { "3P Horizontal Camera", "Change how the camera moves left and right." } },
    { ELanguage::Japanese, { "3Pカメラの[左右:さゆう]", "カメラ[左右:さゆう]の\u200B[動:うご]く[方向:ほうこう]を\u200B[選択:せんたく]できます" } },
    { ELanguage::German,   { "3P Horizontale Kamera", "Ändere wie sich die Kamera nach links und rechts bewegt." } },
    { ELanguage::French,   { "3P Caméra horizontale", "Modifie la rotation horizontale de la caméra." } },
    { ELanguage::Spanish,  { "3P Cámara horizontal", "Cambia cómo se mueve la camara hacia la izquierda y la derecha." } },
    { ELanguage::Italian,  { "3P Telecamera orizzontale", "Modifica come la telecamera si muove da sinistra a destra." } }
};

CONFIG_DEFINE_LOCALE(HorizontalCamera4P)
{
    { ELanguage::English, { "4P Horizontal Camera", "Change how the camera moves left and right." } },
    { ELanguage::Japanese, { "4Pカメラの[左右:さゆう]", "カメラ[左右:さゆう]の\u200B[動:うご]く[方向:ほうこう]を\u200B[選択:せんたく]できます" } },
    { ELanguage::German,   { "4P Horizontale Kamera", "Ändere wie sich die Kamera nach links und rechts bewegt." } },
    { ELanguage::French,   { "4P Caméra horizontale", "Modifie la rotation horizontale de la caméra." } },
    { ELanguage::Spanish,  { "4P Cámara horizontal", "Cambia cómo se mueve la camara hacia la izquierda y la derecha." } },
    { ELanguage::Italian,  { "4P Telecamera orizzontale", "Modifica come la telecamera si muove da sinistra a destra." } }
};


CONFIG_DEFINE_ENUM_LOCALE(ECameraRotationMode)
{
    {
        ELanguage::English,
        {
            { ECameraRotationMode::Normal,  { "NORMAL", "" } },
            { ECameraRotationMode::Reverse, { "REVERSE", "" } }
        }
    },
    {
        ELanguage::Japanese,
        {
            { ECameraRotationMode::Normal,  { "ノーマル", "" } },
            { ECameraRotationMode::Reverse, { "リバース", "" } }
        }
    },
    {
        ELanguage::German,
        {
            { ECameraRotationMode::Normal,  { "NORMAL", "" } },
            { ECameraRotationMode::Reverse, { "INVERTIERT", "" } }
        }
    },
    {
        ELanguage::French,
        {
            { ECameraRotationMode::Normal,  { "NORMALE", "" } },
            { ECameraRotationMode::Reverse, { "INVERSÉE", "" } }
        }
    },
    {
        ELanguage::Spanish,
        {
            { ECameraRotationMode::Normal,  { "NORMAL", "" } },
            { ECameraRotationMode::Reverse, { "INVERTIDO", "" } }
        }
    },
    {
        ELanguage::Italian,
        {
            { ECameraRotationMode::Normal,  { "NORMALE", "" } },
            { ECameraRotationMode::Reverse, { "INVERTITA", "" } }
        }
    }
};



CONFIG_DEFINE_LOCALE(Vibration1P)
{
    { ELanguage::English,  { "1P Vibration", "Toggle controller vibration." } },
    { ELanguage::Japanese, { "1P[振動:しんどう]", "[振動:しんどう]の\u200B[有無:うむ]を\u200B[選択:せんたく]できます" } },
    { ELanguage::German,   { "1P Vibration", "Schalte die Controllervibration an oder aus." } },
    { ELanguage::French,   { "1P Vibration", "Active les vibrations de la manette." } },
    { ELanguage::Spanish,  { "1P Vibración", "Activa o desactiva la vibración del mando." } },
    { ELanguage::Italian,  { "1P Vibrazione", "Attiva/disattiva la vibrazione del controller." } }
};

CONFIG_DEFINE_LOCALE(Vibration2P)
{
    { ELanguage::English,  { "2P Vibration", "Toggle controller vibration." } },
    { ELanguage::Japanese, { "2P[振動:しんどう]", "[振動:しんどう]の\u200B[有無:うむ]を\u200B[選択:せんたく]できます" } },
    { ELanguage::German,   { "2P Vibration", "Schalte die Controllervibration an oder aus." } },
    { ELanguage::French,   { "2P Vibration", "Active les vibrations de la manette." } },
    { ELanguage::Spanish,  { "2P Vibración", "Activa o desactiva la vibración del mando." } },
    { ELanguage::Italian,  { "2P Vibrazione", "Attiva/disattiva la vibrazione del controller." } }
};

CONFIG_DEFINE_LOCALE(Vibration3P)
{
    { ELanguage::English,  { "3P Vibration", "Toggle controller vibration." } },
    { ELanguage::Japanese, { "3P[振動:しんどう]", "[振動:しんどう]の\u200B[有無:うむ]を\u200B[選択:せんたく]できます" } },
    { ELanguage::German,   { "3P Vibration", "Schalte die Controllervibration an oder aus." } },
    { ELanguage::French,   { "3P Vibration", "Active les vibrations de la manette." } },
    { ELanguage::Spanish,  { "3P Vibración", "Activa o desactiva la vibración del mando." } },
    { ELanguage::Italian,  { "3P Vibrazione", "Attiva/disattiva la vibrazione del controller." } }
};

CONFIG_DEFINE_LOCALE(Vibration4P)
{
    { ELanguage::English,  { "4P Vibration", "Toggle controller vibration." } },
    { ELanguage::Japanese, { "4P[振動:しんどう]", "[振動:しんどう]の\u200B[有無:うむ]を\u200B[選択:せんたく]できます" } },
    { ELanguage::German,   { "4P Vibration", "Schalte die Controllervibration an oder aus." } },
    { ELanguage::French,   { "4P Vibration", "Active les vibrations de la manette." } },
    { ELanguage::Spanish,  { "4P Vibración", "Activa o desactiva la vibración del mando." } },
    { ELanguage::Italian,  { "4P Vibrazione", "Attiva/disattiva la vibrazione del controller." } }
};

CONFIG_DEFINE_LOCALE(AllowBackgroundInput)
{
    { ELanguage::English,  { "Allow Background Input", "Allow controller input while the game window is unfocused." } },
    { ELanguage::Japanese, { "バックグラウンド[入力:にゅうりょく]", "フォーカスされていない\u200Bゲームに\u200B[入力:にゅうりょく]できるか\u200B[選択:せんたく]できます" } },
    { ELanguage::German,   { "Erlaube Hintergrundeingaben", "Erlaube Eingaben deines Controllers auch wenn das Spielfenster nicht fokussiert ist." } },
    { ELanguage::French,   { "Manette en arrière plan", "Permet d'utiliser la manette dans le jeu lorsque qu'il n'est pas au premier plan." } },
    { ELanguage::Spanish,  { "Control en segundo plano", "Permite controlar el juego con un mando mientras la ventana esté en segundo plano." } },
    { ELanguage::Italian,  { "Input con la finestra inattiva", "Attiva/disattiva i tasti del controller mentre la finestra è inattiva." } }
};

CONFIG_DEFINE_LOCALE(StickDeadzone1P)
{
    { ELanguage::English,  { "Stick Deadzone", "Adjust analog deadzone." } },
    { ELanguage::Japanese, { "Stick Deadzone", "Adjust analog deadzone." } },
    { ELanguage::German,   { "Stick Deadzone", "Adjust analog deadzone." } },
    { ELanguage::French,   { "Stick Deadzone", "Adjust analog deadzone." } },
    { ELanguage::Spanish,  { "Stick Deadzone", "Adjust analog deadzone." } },
    { ELanguage::Italian,  { "Stick Deadzone", "Adjust analog deadzone." } }
};

// shared by all players
CONFIG_LOCALE g_StickDeadzone2P_locale = g_StickDeadzone1P_locale;
CONFIG_LOCALE g_StickDeadzone3P_locale = g_StickDeadzone1P_locale;
CONFIG_LOCALE g_StickDeadzone4P_locale = g_StickDeadzone1P_locale;

CONFIG_DEFINE_LOCALE(StickSensitivity1P)
{
    { ELanguage::English,  { "Stick Sensitivity", "Adjust analog sensitivity." } },
    { ELanguage::Japanese, { "Stick Sensitivity", "Adjust analog sensitivity." } },
    { ELanguage::German,   { "Stick Sensitivity", "Adjust analog sensitivity." } },
    { ELanguage::French,   { "Stick Sensitivity", "Adjust analog sensitivity." } },
    { ELanguage::Spanish,  { "Stick Sensitivity", "Adjust analog sensitivity." } },
    { ELanguage::Italian,  { "Stick Sensitivity", "Adjust analog sensitivity." } }
};

// shared by all players
CONFIG_LOCALE g_StickSensitivity2P_locale = g_StickSensitivity1P_locale;
CONFIG_LOCALE g_StickSensitivity3P_locale = g_StickSensitivity1P_locale;
CONFIG_LOCALE g_StickSensitivity4P_locale = g_StickSensitivity1P_locale;

CONFIG_DEFINE_LOCALE(MasterVolume)
{
    { ELanguage::English, { "Master Volume", "Adjust the overall volume." } },
    { ELanguage::Japanese, { "マスターボリューム", "[全体:ぜんたい]ボリュームの\u200B[大:おお]きさを\u200B[調整:ちょうせい]できます" } },
    { ELanguage::German,   { "Gesamtlautstärke", "Passe die Gesamtlautstärke an." } },
    { ELanguage::French,   { "Volume général", "Réglage du volume général." } },
    { ELanguage::Spanish,  { "Volumen maestro", "Ajusta el volumen general." } },
    { ELanguage::Italian,  { "Volume principale", "Regola il volume principale" } }
};

CONFIG_DEFINE_LOCALE(MusicVolume)
{
    { ELanguage::English,  { "Music Volume", "Adjust the volume for music." } },
    { ELanguage::Japanese, { "BGMボリューム", "BGMボリュームの\u200B[大:おお]きさを\u200B[調整:ちょうせい]できます" } },
    { ELanguage::German,   { "Musiklautstärke", "Passe die Lautstärke der Musik an." } },
    { ELanguage::French,   { "Volume de la musique", "Réglage du volume de la musique." } },
    { ELanguage::Spanish,  { "Volumen de la música", "Ajusta el volumen de la música." } },
    { ELanguage::Italian,  { "Volume musica di sottofondo", "Regola il volume della musica di sottofondo." } }
};

CONFIG_DEFINE_LOCALE(EffectsVolume)
{
    { ELanguage::English,  { "Effects Volume", "Adjust the volume for sound effects." } },
    { ELanguage::Japanese, { "SEボリューム", "SEボリュームの\u200B[大:おお]きさを\u200B[調整:ちょうせい]できます" } },
    { ELanguage::German,   { "Soundeffektlautstärke", "Passe die Lautstärke der Soundeffekte an." } },
    { ELanguage::French,   { "Volume des effets sonores", "Réglage du volume des effets sonores." } },
    { ELanguage::Spanish,  { "Volumen de efectos", "Ajusta el volumen de los efectos de sonido." } },
    { ELanguage::Italian,  { "Volume effetti sonori", "Regola il volume degli effetti sonori." } }
};

CONFIG_DEFINE_LOCALE(VoicesVolume)
{
    { ELanguage::English,  { "Voices Volume", "Adjust the volume for voices." } },
    { ELanguage::Japanese, { "ボイスボリューム", "ボイスボリュームの\u200B[大:おお]きさを\u200B[調整:ちょうせい]できます" } },
    { ELanguage::German,   { "Stimmenlautstärke", "Passe die Lautstärke der Stimmen an." } },
    { ELanguage::French,   { "Volume des voix", "Réglage du volume des voix." } },
    { ELanguage::Spanish,  { "Volumen de voces", "Ajusta el volumen de las voces." } },
    { ELanguage::Italian,  { "Volume delle voci", "Regola il volume delle voci." } },
};

CONFIG_DEFINE_LOCALE(CutsceneVolume)
{
    { ELanguage::English,  { "Cutscene Volume", "Adjust the volume for cutscenes." } },
    { ELanguage::Japanese, { "Cutscene Volume", "Adjust the volume for cutscenes." } },
    { ELanguage::German,   { "Cutscene Volume", "Adjust the volume for cutscenes." } },
    { ELanguage::French,   { "Cutscene Volume", "Adjust the volume for cutscenes." } },
    { ELanguage::Spanish,  { "Cutscene Volume", "Adjust the volume for cutscenes." } },
    { ELanguage::Italian,  { "Cutscene Volume", "Adjust the volume for cutscenes." } }
};

// Japanese Notes: This localization should include furigana.
CONFIG_DEFINE_LOCALE(MusicAttenuation)
{
    { ELanguage::English,  { "Music Attenuation", "Fade out the game's music when external media is playing." } },
    { ELanguage::Japanese, { "BGM[減衰:げんすい]", "[外部:がいぶ]メディアを\u200B[再生:さいせい]すると\u200Bゲームの\u200B[音楽:おんがく]を\u200Bフェードアウト\u200Bします" } },
    { ELanguage::German,   { "Musikdämpfung", "Stelle die Musik des Spiels stumm während externe Medien abgespielt werden." } },
    { ELanguage::French,   { "Atténuation audio", "Abaisse le volume des musiques du jeu lorsqu'un média externe est en cours de lecture." } },
    { ELanguage::Spanish,  { "Atenuación de música", "Atenúa la música del juego cuando un reproductor multimedia se encuentra activo." } },
    { ELanguage::Italian,  { "Attenuazione musica", "Abbassa il volume della musica di sottofondo quando un'altra applicazione riproduce dei suoni." } }
};

CONFIG_DEFINE_LOCALE(MuteOnFocusLost)
{
    { ELanguage::English,  { "Mute on Focus Lost", "Mute the game's audio when the window is not in focus." } },
    { ELanguage::Japanese, { "フォーカスロストミュート", "ウィンドウがフォーカスされていないときにゲームのオーディオをミュートします" } },
    { ELanguage::German,   { "Stummstellen wenn nicht fokussiert", "Stellt das Audio des Spiels stumm solange das Fenster nicht im Fokus ist." } },
    { ELanguage::French,   { "Muet si fenêtre inactive", "Coupe le son du jeu lorsque la fenêtre n'est pas active." } },
    { ELanguage::Spanish,  { "Silenciar al perder el foco", "Silencia el audio del juego cuando la ventana no esté en primer plano." } },
    { ELanguage::Italian,  { "Audio muto con finestra inattiva", "Disattiva l'audio del gioco quando la finestra non è attiva." } }
};

// TODO: I changed this completely; Sora to review before shipping to translators
CONFIG_DEFINE_LOCALE(IdleChaosPowersMusic)
{
    { ELanguage::English,  { "Chaos Power Music", "Play the theme when either chaos gauge is full. 'OFF' will continue to play the stage music instead." } },
    { ELanguage::Japanese, { "Chaos Power Music", "Play the theme when either chaos gauge is full. 'OFF' will continue to play the stage music instead." } },
    { ELanguage::German,   { "Chaos Power Music", "Play the theme when either chaos gauge is full. 'OFF' will continue to play the stage music instead." } },
    { ELanguage::French,   { "Chaos Power Music", "Play the theme when either chaos gauge is full. 'OFF' will continue to play the stage music instead." } },
    { ELanguage::Spanish,  { "Chaos Power Music", "Play the theme when either chaos gauge is full. 'OFF' will continue to play the stage music instead." } },
    { ELanguage::Italian,  { "Chaos Power Music", "Play the theme when either chaos gauge is full. 'OFF' will continue to play the stage music instead." } }
};

// TODO: This was changed slightly, retranslate FR
CONFIG_DEFINE_LOCALE(ChaosPowerLEDSync)
{
    { ELanguage::English,  { "Chaos Power LED Sync", "Sync controller LED color with the active Chaos Power." } },
    { ELanguage::Japanese, { "Chaos Power LED Sync", "Sync controller LED color with the active Chaos Power." } },
    { ELanguage::German,   { "Chaos Power LED Sync", "Sync controller LED color with the active Chaos Power." } },
    { ELanguage::French,   { "Chaos Power LED Sync", "Synchronise la couleur des LED de la manette avec le Chaos Power actif." } },
    { ELanguage::Spanish,  { "Chaos Power LED Sync", "Sync controller LED color with the active Chaos Power." } },
    { ELanguage::Italian,  { "Chaos Power LED Sync", "Sync controller LED color with the active Chaos Power." } }
};

// TODO: Not yet used, but will add this before release so adding it here before translators start
CONFIG_DEFINE_LOCALE(ControllerLEDBrightness)
{
    { ELanguage::English,  { "Controller LED Brightness", "LED brightness for compatible controllers." } },
    { ELanguage::Japanese, { "Controller LED Brightness", "LED brightness for compatible controllers." } },
    { ELanguage::German,   { "Controller LED Brightness", "LED brightness for compatible controllers." } },
    { ELanguage::French,   { "Controller LED Brightness", "LED brightness for compatible controllers." } },
    { ELanguage::Spanish,  { "Controller LED Brightness", "LED brightness for compatible controllers." } },
    { ELanguage::Italian,  { "Controller LED Brightness", "LED brightness for compatible controllers." } }
};

// Note: English was updated to match OG game, retranslate this
CONFIG_DEFINE_LOCALE(VoiceLanguage)
{
    { ELanguage::English, { "Voice Language", "Change the spoken language." } },
    { ELanguage::Japanese, { "[音声言語:おんせいげんご]", "ゲーム[内:ない]の\u200B[音声言語:おんせいげんご]を\u200B[変更:へんこう]できます" } },
    { ELanguage::German,   { "Stimmeinstellung", "Ändere die Sprache, die für Charakterstimmen benutzt wird." } },
    { ELanguage::French,   { "Langue des voix", "Modifie la langue utilisée pour la voix des personnages." } },
    { ELanguage::Spanish,  { "Idioma de voz", "Cambia el idioma utilizado para las voces de los personajes." } },
    { ELanguage::Italian, { "Lingua delle voci", "Modifica la lingua utilizzata per le voci dei personaggi." } }
};

CONFIG_DEFINE_LOCALE(Subtitles)
{
    { ELanguage::English,  { "Subtitles", "Show subtitles during dialogue." } },
    { ELanguage::Japanese, { "[字幕:じまく]", "[字幕:じまく]の[表示:ひょうじ]を\u200B[選択:せんたく]できます" } },
    { ELanguage::German,   { "Untertitel", "Zeige Untertitel bei Dialogen." } },
    { ELanguage::French,   { "Sous-titres", "Affiche les sous-titres pendant les dialogues." } },
    { ELanguage::Spanish,  { "Subtítulos", "Muestra subtítulos durante los diálogos." } },
    { ELanguage::Italian,  { "Sottotitoli", "Mostra i sottotitoli durante i dialoghi." } }
};

CONFIG_DEFINE_LOCALE(GraphicsAPI)
{
    { ELanguage::English,  { "Rendering Backend", "Set graphics API used for rendering. Only change this if you experience visual issues." } },
    { ELanguage::Japanese, { "Rendering Backend", "Set graphics API used for rendering. Only change this if you experience visual issues." } },
    { ELanguage::German,   { "Rendering Backend", "Set graphics API used for rendering. Only change this if you experience visual issues." } },
    { ELanguage::French,   { "Rendering Backend", "Set graphics API used for rendering. Only change this if you experience visual issues." } },
    { ELanguage::Spanish,  { "Rendering Backend", "Set graphics API used for rendering. Only change this if you experience visual issues." } },
    { ELanguage::Italian,  { "Rendering Backend", "Set graphics API used for rendering. Only change this if you experience visual issues." } }
};

CONFIG_DEFINE_ENUM_LOCALE(EGraphicsAPI)
{
    {
        ELanguage::English,
        {
            { EGraphicsAPI::Auto, { "AUTO", "Render using recommended backend." } },
#ifdef SHADOW_RECOMP_D3D12
            { EGraphicsAPI::D3D12, { "D3D12", "Render using Direct3D 12." } },
#endif
            { EGraphicsAPI::Vulkan, { "VULKAN", "Render using Vulkan." } }
        }
    },
    {
        ELanguage::Japanese,
        {
            { EGraphicsAPI::Auto, { "AUTO", "Render using recommended backend." } },
#ifdef SHADOW_RECOMP_D3D12
            { EGraphicsAPI::D3D12, { "D3D12", "Render using Direct3D 12." } },
#endif
            { EGraphicsAPI::Vulkan, { "VULKAN", "Render using Vulkan." } }
        }
    },
    {
        ELanguage::German,
        {
            { EGraphicsAPI::Auto, { "AUTO", "Render using recommended backend." } },
#ifdef SHADOW_RECOMP_D3D12
            { EGraphicsAPI::D3D12, { "D3D12", "Render using Direct3D 12." } },
#endif
            { EGraphicsAPI::Vulkan, { "VULKAN", "Render using Vulkan." } }
        }
    },
    {
        ELanguage::French,
        {
            { EGraphicsAPI::Auto, { "AUTO", "Render using recommended backend." } },
#ifdef SHADOW_RECOMP_D3D12
            { EGraphicsAPI::D3D12, { "D3D12", "Render using Direct3D 12." } },
#endif
            { EGraphicsAPI::Vulkan, { "VULKAN", "Render using Vulkan." } }
        }
    },
    {
        ELanguage::Spanish,
        {
            { EGraphicsAPI::Auto, { "AUTO", "Render using recommended backend." } },
#ifdef SHADOW_RECOMP_D3D12
            { EGraphicsAPI::D3D12, { "D3D12", "Render using Direct3D 12." } },
#endif
            { EGraphicsAPI::Vulkan, { "VULKAN", "Render using Vulkan." } }
        }
    },
    {
        ELanguage::Italian,
        {
            { EGraphicsAPI::Auto, { "AUTO", "Render using recommended backend." } },
#ifdef SHADOW_RECOMP_D3D12
            { EGraphicsAPI::D3D12, { "D3D12", "Render using Direct3D 12." } },
#endif
            { EGraphicsAPI::Vulkan, { "VULKAN", "Render using Vulkan." } }
        }
    }
};

CONFIG_DEFINE_LOCALE(WindowSize)
{
    { ELanguage::English,  { "Resolution", "Adjust the resolution of the game in windowed mode." } },
    { ELanguage::Japanese, { "解像度", "ウィンドウ\u200Bモードでの\u200Bゲームの\u200B[解像度:かいぞうど]を\u200B[調整:ちょうせい]できます" } },
    { ELanguage::German,   { "Auflösung", "Ändere die Auflösung des Spiels im Fenstermodus." } },
    { ELanguage::French,   { "Résolution", "Modifie la résolution du jeu en mode fenêtré." } },
    { ELanguage::Spanish,  { "Resolución", "Ajusta la resolución del juego en modo ventana." } },
    { ELanguage::Italian,  { "Risoluzione", "Regola la risoluzione del gioco in modalità finestra." } }
};

CONFIG_DEFINE_LOCALE(Monitor)
{
    { ELanguage::English,  { "Monitor", "Change which monitor to display the game on." } },
    { ELanguage::Japanese, { "モニター[選択:せんたく]", "ゲームを[表示:ひょうじ]する\u200Bモニターを\u200B[変更:へんこう]できます" } },
    { ELanguage::German,   { "Monitor", "Ändere auf welchem Monitor das Spiel angezeigt wird." } },
    { ELanguage::French,   { "Moniteur", "Change le moniteur sur lequel le jeu sera affiché." } },
    { ELanguage::Spanish,  { "Pantalla", "Cambia la pantalla en la cuál se muestra el juego." } },
    { ELanguage::Italian,  { "Schermo", "Cambia lo schermo su cui visualizzare il gioco." } }
};

CONFIG_DEFINE_LOCALE(AspectRatio)
{
    { ELanguage::English,  { "Aspect Ratio", "Change the aspect ratio." } },
    { ELanguage::Japanese, { "アスペクト[比:ひ]", "アスペクト[比:ひ]を\u200B[変更:へんこう]できます" } },
    { ELanguage::German,   { "Seitenverhältnis", "Verändere das Seitenverhältnis." } },
    { ELanguage::French,   { "Format d'image", "Modifie le format d'image." } },
    { ELanguage::Spanish,  { "Relación de aspecto", "Cambia la relación de aspecto." } },
    { ELanguage::Italian,  { "Rapporto d'aspetto", "Modifica il rapporto d'aspetto." } }
};

CONFIG_DEFINE_ENUM_LOCALE(EAspectRatio)
{
    {
        ELanguage::English,
        {
            { EAspectRatio::Auto, { "AUTO", "Dynamically adjusts the aspect ratio to the window size." } },
            { EAspectRatio::Original, { "4:3 (Original)", "The game's original aspect ratio." } },
            { EAspectRatio::Wide16x9, { "16:9", "Force a 16:9 aspect ratio." } },
            { EAspectRatio::Wide16x10, { "16:10", "Force a 16:10 aspect ratio." } },
            { EAspectRatio::Narrow5x4, { "5:4", "Force a 5:4 aspect ratio." } },
            { EAspectRatio::Wide21x9, { "21:9", "Force a 21:9 aspect ratio." } }
        }
    },
    {
        ELanguage::Japanese,
        {
            { EAspectRatio::Auto, { "自動", "TODO" } },
            { EAspectRatio::Original, { "4:3 (Original)", "TODO" } },
            { EAspectRatio::Wide16x9, { "16:9", "TODO" } },
            { EAspectRatio::Wide16x10, { "16:10", "TODO" } },
            { EAspectRatio::Narrow5x4, { "5:4", "TODO" } },
            { EAspectRatio::Wide21x9, { "21:9", "TODO" } }
        }
    },
    {
        ELanguage::German,
        {
            { EAspectRatio::Auto, { "AUTO", "TODO" } },
            { EAspectRatio::Original, { "4:3 (Original)", "TODO" } },
            { EAspectRatio::Wide16x9, { "16:9", "TODO" } },
            { EAspectRatio::Wide16x10, { "16:10", "TODO" } },
            { EAspectRatio::Narrow5x4, { "5:4", "TODO" } },
            { EAspectRatio::Wide21x9, { "21:9", "TODO" } }
        }
    },
    {
        ELanguage::French,
        {
            { EAspectRatio::Auto, { "AUTO", "TODO" } },
            { EAspectRatio::Original, { "4:3 (Original)", "TODO" } },
            { EAspectRatio::Wide16x9, { "16:9", "TODO" } },
            { EAspectRatio::Wide16x10, { "16:10", "TODO" } },
            { EAspectRatio::Narrow5x4, { "5:4", "TODO" } },
            { EAspectRatio::Wide21x9, { "21:9", "TODO" } }
        }
    },
    {
        ELanguage::Spanish,
        {
            { EAspectRatio::Auto, { "AUTO", "TODO" } },
            { EAspectRatio::Original, { "4:3 (Original)", "TODO" } },
            { EAspectRatio::Wide16x9, { "16:9", "TODO" } },
            { EAspectRatio::Wide16x10, { "16:10", "TODO" } },
            { EAspectRatio::Narrow5x4, { "5:4", "TODO" } },
            { EAspectRatio::Wide21x9, { "21:9", "TODO" } }
        }
    },
    {
        ELanguage::Italian,
        {
            { EAspectRatio::Auto, { "AUTO", "TODO" } },
            { EAspectRatio::Original, { "4:3 (Originale)", "TODO" } },
            { EAspectRatio::Wide16x9, { "16:9", "TODO" } },
            { EAspectRatio::Wide16x10, { "16:10", "TODO" } },
            { EAspectRatio::Narrow5x4, { "5:4", "TODO" } },
            { EAspectRatio::Wide21x9, { "21:9", "TODO" } }
        }
    }
};

CONFIG_DEFINE_LOCALE(FMVAspectRatio)
{
    { ELanguage::English,  { "FMV Aspect Ratio", "Change how full motion videos (pre-rendered cutscenes) fit the display." } },
    { ELanguage::Japanese, { "FMV Aspect Ratio", "Change how full motion videos (pre-rendered cutscenes) fit the display." } },
    { ELanguage::German,   { "FMV Aspect Ratio", "Change how full motion videos (pre-rendered cutscenes) fit the display." } },
    { ELanguage::French,   { "FMV Aspect Ratio", "Change how full motion videos (pre-rendered cutscenes) fit the display." } },
    { ELanguage::Spanish,  { "FMV Aspect Ratio", "Change how full motion videos (pre-rendered cutscenes) fit the display." } },
    { ELanguage::Italian,  { "FMV Aspect Ratio", "Change how full motion videos (pre-rendered cutscenes) fit the display." } }
};

// Japanese Notes: This localization should include furigana in its description.
CONFIG_DEFINE_ENUM_LOCALE(EFMVAspectRatio)
{
    {
        ELanguage::English,
        {
            { EFMVAspectRatio::Stretched, { "STRETCHED", "The video is stretched to fill the display." } },
            { EFMVAspectRatio::Fit,       { "FIT", "The video keeps its original aspect ratio with black borders." } },
            { EFMVAspectRatio::Fill,      { "FILL", "The video is cropped and zoomed in with its original aspect ratio." } },
        }
    },
    {
        ELanguage::Japanese,
        {
            { EFMVAspectRatio::Stretched, { "STRETCHED", "The video is stretched to fill the display." } },
            { EFMVAspectRatio::Fit,       { "FIT", "The video keeps its original aspect ratio with black borders." } },
            { EFMVAspectRatio::Fill,      { "FILL", "The video is cropped and zoomed in with its original aspect ratio." } },
        }
    },
    {
        ELanguage::German,
        {
            { EFMVAspectRatio::Stretched, { "STRETCHED", "The video is stretched to fill the display." } },
            { EFMVAspectRatio::Fit,       { "FIT", "The video keeps its original aspect ratio with black borders." } },
            { EFMVAspectRatio::Fill,      { "FILL", "The video is cropped and zoomed in with its original aspect ratio." } },
        }
    },
    {
        ELanguage::French,
        {
            { EFMVAspectRatio::Stretched, { "STRETCHED", "The video is stretched to fill the display." } },
            { EFMVAspectRatio::Fit,       { "FIT", "The video keeps its original aspect ratio with black borders." } },
            { EFMVAspectRatio::Fill,      { "FILL", "The video is cropped and zoomed in with its original aspect ratio." } },
        }
    },
    {
        ELanguage::Spanish,
        {
            { EFMVAspectRatio::Stretched, { "STRETCHED", "The video is stretched to fill the display." } },
            { EFMVAspectRatio::Fit,       { "FIT", "The video keeps its original aspect ratio with black borders." } },
            { EFMVAspectRatio::Fill,      { "FILL", "The video is cropped and zoomed in with its original aspect ratio." } },
        }
    },
    {
        ELanguage::Italian,
        {
            { EFMVAspectRatio::Stretched, { "STRETCHED", "The video is stretched to fill the display." } },
            { EFMVAspectRatio::Fit,       { "FIT", "The video keeps its original aspect ratio with black borders." } },
            { EFMVAspectRatio::Fill,      { "FILL", "The video is cropped and zoomed in with its original aspect ratio." } },
        }
    }
};

CONFIG_DEFINE_LOCALE(ResolutionScale)
{
    { ELanguage::English,  { "Resolution Scale", "Adjust the internal resolution of the game." } },
    { ELanguage::Japanese, { "[解像度:かいぞうど]スケール", "ゲームの\u200B[内部解像度:ないぶかいぞうど]を\u200B[調整:ちょうせい]できます" } },
    { ELanguage::German,   { "Rendering-Auflösung", "Passe die Auflösung der internen Darstellung an." } },
    { ELanguage::French,   { "Échelle de rendu", "Modifie la résolution interne du jeu." } },
    { ELanguage::Spanish,  { "Escala de resolución", "Ajusta la resolución interna del juego." } },
    { ELanguage::Italian,  { "Scala risoluzione", "Regola la risoluzione interna del gioco." } }
};

CONFIG_DEFINE_LOCALE(Fullscreen)
{
    { ELanguage::English,  { "Fullscreen", "Toggle between borderless fullscreen or windowed mode." } },
    { ELanguage::Japanese, { "フルスクリーン", "ボーダーレス\u200Bフルスクリーンか\u200Bウィンドウモードを\u200B[選択:せんたく]できます" } },
    { ELanguage::German,   { "Vollbild", "Wechsle zwischen dem randlosen Vollbildmodus und dem Fenstermodus." } },
    { ELanguage::French,   { "Plein écran", "Alterne entre le mode plein écran sans bordures et le mode fenêtré." } },
    { ELanguage::Spanish,  { "Pantalla completa", "Cambia entre modo de pantalla completa o ventana." } },
    { ELanguage::Italian,  { "Schermo pieno", "Attiva/disattiva tra modalità finestra senza cornice e modalità finestra." } }
};

CONFIG_DEFINE_LOCALE(VSync)
{
    { ELanguage::English,  { "V-Sync", "Synchronize the game to the refresh rate of the display to prevent screen tearing." } },
    { ELanguage::Japanese, { "[垂直同期:すいちょくどうき]", "[垂直同期:すいちょくどうき]の\u200B[設定:せってい]を\u200B[変更:へんこう]できます" } },
    { ELanguage::German,   { "V-Sync", "Synchronisiere das Spiel mit der Bildwiederholrate deines Bildschirms um Bildverzerrungen zu vermeiden." } },
    { ELanguage::French,   { "V-Sync", "Synchronise le jeu avec la fréquence de rafraîchissement de l'écran pour éviter le screen tearing." } },
    { ELanguage::Spanish,  { "V-Sync", "Sincroniza el juego a la tasa de refresco de la pantalla para prevenir el rasgado de la imagen." } },
    { ELanguage::Italian,  { "V-Sync", "Sincronizza il gioco con la frequenza d'aggiornamento del display per evitare lo screen tearing." } }
};

CONFIG_DEFINE_LOCALE(FPS)
{
    { ELanguage::English,  { "FPS", "Set the max frame rate the game can run at. WARNING: this may introduce glitches at frame rates higher than 60 FPS." } },
    { ELanguage::Japanese, { "フレームレート[上限:じょうげん]", "ゲームの\u200B[最大:さいだい]フレームレートを\u200B[設定:せってい]できます [警告:けいこく]: 60 FPSを\u200B[超:こ]えるフレームレートで\u200B[不具合:ふぐあい]が\u200B[発生:はっせい]する\u200B[可能性:かのうせい]が\u200Bあります" } },
    { ELanguage::German,   { "FPS", "Setze die maximale Anzahl der Bilder pro Sekunde, die das Spiel darstellen kann. WARNUNG: Das Spiel kann bei höheren FPS als 60 ungewolltes Verhalten aufweisen." } },
    { ELanguage::French,   { "IPS", "Détermine la fréquence d'images maximale du jeu. ATTENTION : cela peut entraîner des problèmes à des taux de rafraîchissement supérieurs à 60 IPS." } },
    { ELanguage::Spanish,  { "FPS", "Establece la tasa de fotogramas máxima a la que puede correr el juego. ADVERTENCIA: esto puede introducir fallos en tasas mayores a 60 FPS." } },
    { ELanguage::Italian,  { "FPS", "Imposta il frame rate massimo del gioco. ATTENZIONE: questa opzione può causare dei glitch a frame rate più alti di 60 FPS." } }
};

CONFIG_DEFINE_LOCALE(CutsceneFramerate)
{
    { ELanguage::English,  { "Cutscene Framerate", "Set the frame rate of in-game cutscenes." } },
    { ELanguage::Japanese, { "Cutscene Framerate", "Set the frame rate of in-game cutscenes." } },
    { ELanguage::German,   { "Cutscene Framerate", "Set the frame rate of in-game cutscenes." } },
    { ELanguage::French,   { "Cutscene Framerate", "Set the frame rate of in-game cutscenes." } },
    { ELanguage::Spanish,  { "Cutscene Framerate", "Set the frame rate of in-game cutscenes." } },
    { ELanguage::Italian,  { "Cutscene Framerate", "Set the frame rate of in-game cutscenes." } }
};

CONFIG_DEFINE_ENUM_LOCALE(ECutsceneFramerate)
{
    {
        ELanguage::English,
        {
            { ECutsceneFramerate::Original, { "ORIGINAL", "In-game cutscenes run at 30 FPS." } },
            { ECutsceneFramerate::Unlocked, { "UNLOCKED", "In-game cutscenes run at target framerate." } }
        }
    },
    {
        ELanguage::Japanese,
        {
            { ECutsceneFramerate::Original, { "ORIGINAL", "In-game cutscenes run at 30 FPS." } },
            { ECutsceneFramerate::Unlocked, { "UNLOCKED", "In-game cutscenes run at target framerate." } }
        }
    },
    {
        ELanguage::German,
        {
            { ECutsceneFramerate::Original, { "ORIGINAL", "In-game cutscenes run at 30 FPS." } },
            { ECutsceneFramerate::Unlocked, { "UNLOCKED", "In-game cutscenes run at target framerate." } }
        }
    },
    {
        ELanguage::French,
        {
            { ECutsceneFramerate::Original, { "ORIGINAL", "In-game cutscenes run at 30 FPS." } },
            { ECutsceneFramerate::Unlocked, { "UNLOCKED", "In-game cutscenes run at target framerate." } }
        }
    },
    {
        ELanguage::Spanish,
        {
            { ECutsceneFramerate::Original, { "ORIGINAL", "In-game cutscenes run at 30 FPS." } },
            { ECutsceneFramerate::Unlocked, { "UNLOCKED", "In-game cutscenes run at target framerate." } }
        }
    },
    {
        ELanguage::Italian,
        {
            { ECutsceneFramerate::Original, { "ORIGINAL", "In-game cutscenes run at 30 FPS." } },
            { ECutsceneFramerate::Unlocked, { "UNLOCKED", "In-game cutscenes run at target framerate." } }
        }
    }
};

CONFIG_DEFINE_LOCALE(ShowFPS)
{
    { ELanguage::English,  { "Show FPS", "Shows the framerate counter." } },
    { ELanguage::Japanese, { "Show FPS", "Shows the framerate counter." } },
    { ELanguage::German,   { "Show FPS", "Shows the framerate counter." } },
    { ELanguage::French,   { "Show FPS", "Shows the framerate counter." } },
    { ELanguage::Spanish,  { "Show FPS", "Shows the framerate counter." } },
    { ELanguage::Italian,  { "Show FPS", "Shows the framerate counter." } }
};

CONFIG_DEFINE_LOCALE(Brightness)
{
    { ELanguage::English, { "Brightness", "Adjust the brightness level." } },
    { ELanguage::Japanese, { "[明:あか]るさの[設定:せってい]", "Adjust the brightness level." } },
    { ELanguage::German,   { "Helligkeit", "Adjust the brightness level." } },
    { ELanguage::French,   { "Luminosité", "Adjust the brightness level." } },
    { ELanguage::Spanish,  { "Brillo", "Adjust the brightness level." } },
    { ELanguage::Italian,  { "Luminosità", "Adjust the brightness level." } }
};

CONFIG_DEFINE_LOCALE(TextureFiltering)
{
    { ELanguage::English, { "Texture Filtering", "Change the quality of the filtering used for textures." } },
    { ELanguage::Japanese, { "GIテクスチャフィルタリング", "GIテクスチャに\u200B[使用:しよう]する\u200Bフィルタリングの\u200B[品質:ひんしつ]を\u200B[変更:へんこう]できます" } },
    { ELanguage::German,   { "GI Texturfilterung", "Ändere die Qualität der Filterung für Global-Illumination-Texturen." } },
    { ELanguage::French,   { "Filtre des textures", "Modifie la qualité du filtre utilisé pour les textures." } },
    { ELanguage::Spanish,  { "Filtrado de texturas de GI", "Cambia la calidad del filtrado utilizado en las texturas de iluminación global." } },
    { ELanguage::Italian,  { "Filtraggio delle texture GI", "Regola la qualità del sistema di filtraggio utilizzato per le texture dell'illuminazione." } }
};

CONFIG_DEFINE_ENUM_LOCALE(ETextureFiltering)
{
    {
        ELanguage::English,
        {
            { ETextureFiltering::Auto, { "AUTO", "" } },
            { ETextureFiltering::Linear,  { "LINEAR", "" } }
        }
    },
    {
        ELanguage::Japanese,
        {
            { ETextureFiltering::Auto, { "AUTO", "" } },
            { ETextureFiltering::Linear,  { "LINEAR", "" } }
        }
    },
    {
        ELanguage::German,
        {
            { ETextureFiltering::Auto, { "AUTO", "" } },
            { ETextureFiltering::Linear,  { "LINEAR", "" } }
        }
    },
    {
        ELanguage::French,
        {
            { ETextureFiltering::Auto, { "AUTO", "" } },
            { ETextureFiltering::Linear,  { "LINÉAIRE", "" } }
        }
    },
    {
        ELanguage::Spanish,
        {
            { ETextureFiltering::Auto, { "AUTO", "" } },
            { ETextureFiltering::Linear,  { "LINEAR", "" } }
        }
    },
    {
        ELanguage::Italian,
        {
            { ETextureFiltering::Auto, { "AUTO", "" } },
            { ETextureFiltering::Linear,  { "LINEAR", "" } }
        }
    }
};

CONFIG_DEFINE_LOCALE(BloomIntensity)
{
    { ELanguage::English, { "Bloom Intensity", "Adjust the intensity of the bloom effect." } },
    { ELanguage::Japanese, { "Bloom Intensity", "Adjust the intensity of the bloom effect." } },
    { ELanguage::German,   { "Bloom-Intensität", "Passe die Intensität des Bloom-Effekts an." } },
    { ELanguage::French,   { "Intensité du bloom", "Réglez l'intensité de l'effet du bloom." } },
    { ELanguage::Spanish,  { "Intensidad del bloom", "Ajusta la intensidad del efecto de bloom." } },
    { ELanguage::Italian,  { "Intensità del bloom", "Regola l'intensità dell'effetto bloom." } }
};

// TODO: Retranslate this one, I changed it
CONFIG_DEFINE_LOCALE(EnablePartnerIntro)
{
    { ELanguage::English,  { "Partner Intro", "Show intro when meeting new partners." } },
    { ELanguage::Japanese, { "Partner Intro", "Show intro when meeting new partners." } },
    { ELanguage::German,   { "Partner Intro", "Show intro when meeting new partners." } },
    { ELanguage::French,   { "Intro du Partenaire", "Afficher l'introduction du partenaire lorsqu'il est rencontré pour la première fois." } },
    { ELanguage::Spanish,  { "Partner Intro", "Show intro when meeting new partners." } },
    { ELanguage::Italian,  { "Partner Intro", "Show intro when meeting new partners." } }
};

CONFIG_DEFINE_LOCALE(MaxHUDWidth)
{
    { ELanguage::English,  { "HUD Width", "Constrain HUD to a chosen aspect ratio. Primarily for ultrawide monitors." } },
    { ELanguage::Japanese, { "HUD Width", "Constrain HUD to a chosen aspect ratio. Primarily for ultrawide monitors." } },
    { ELanguage::German,   { "HUD Width", "Constrain HUD to a chosen aspect ratio. Primarily for ultrawide monitors." } },
    { ELanguage::French,   { "HUD Width", "Constrain HUD to a chosen aspect ratio. Primarily for ultrawide monitors." } },
    { ELanguage::Spanish,  { "HUD Width", "Constrain HUD to a chosen aspect ratio. Primarily for ultrawide monitors." } },
    { ELanguage::Italian,  { "HUD Width", "Constrain HUD to a chosen aspect ratio. Primarily for ultrawide monitors." } }
};
