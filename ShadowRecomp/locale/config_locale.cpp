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

CONFIG_DEFINE_LOCALE(Language)
{
    { ELanguage::English, { "Language", "Change the language used for text and logos." } },
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
    { ELanguage::English, { "Hints", "Show hints during gameplay." } },
    { ELanguage::Japanese, { "ヒントリング", "ゲーム[内:ない]に\u200Bヒントリングを\u200B[表示:ひょうじ]するか\u200B[選択:せんたく]できます" } },
    { ELanguage::German,   { "Hinweise", "Zeige Hinweise während des Spiels." } },
    { ELanguage::French,   { "Indices", "Affiche les indices pendant le jeu." } },
    { ELanguage::Spanish,  { "Pistas", "Muestra pistas durante el juego." } },
    { ELanguage::Italian,  { "Indizi", "Mostra degli indizzi durante il gioco." } }
};

CONFIG_DEFINE_LOCALE(AchievementNotifications)
{
    { ELanguage::English, { "Achievement Notifications", "Show notifications for unlocking achievements.\n\nAchievements will still be rewarded with notifications disabled." } },
    { ELanguage::Japanese, { "[実績通知:じっせきつうち]", "[実績通知:じっせきつうち]の\u200B[有無:うむ]を\u200B[選択:せんたく]できます\n\nオフにしても\u200B[実績:じっせき]は\u200B[付与:ふよ]されます" } },
    { ELanguage::German,   { "Erfolgsbenachrichtigungen", "Zeige Benachrichtigungen für das Freischalten von Erfolgen.\n\nErfolge werden weiterhin freigeschaltet, auch wenn die Benachrichtigungen ausgeschaltet sind." } },
    { ELanguage::French,   { "Notification des succès", "Affiche les notifications pour le déverrouillage des succès.\n\nLes succès seront toujours obtenus même si les notifications sont désactivées." } },
    { ELanguage::Spanish,  { "Notificaciones de logros", "Muestra notificaciones al desbloquear logros.\n\nLos logros se seguirán obteniendo aunque las notificaciones estén desactivadas." } },
    { ELanguage::Italian,  { "Notifiche obiettivi", "Mostra delle notifiche quando sblocchi degli obiettivi.\n\nGli obiettivi verranno comunque assegnati anche con le notifiche disattivate." } }
};

CONFIG_DEFINE_LOCALE(ConfirmationPrompt)
{
    { ELanguage::English,  { "Confirmation Prompts", "Shows confirmation prompts when loading a save file, exiting to the menu, restarting, and in other similar situations.\n\nDisabling this option replicates the behavior of the GameCube version." } },
    { ELanguage::Japanese, { "Confirmation Prompts", "Shows confirmation prompts when loading a save file, exiting to the menu, restarting, and in other similar situations.\n\nDisabling this option replicates the behavior of the GameCube version." } },
    { ELanguage::German,   { "Confirmation Prompts", "Shows confirmation prompts when loading a save file, exiting to the menu, restarting, and in other similar situations.\n\nDisabling this option replicates the behavior of the GameCube version." } },
    { ELanguage::French,   { "Confirmation Prompts", "Affiche les messages de confirmation lors du chargement d'une sauvegarde, du retour au menu, du redémarrage et dans d'autres situations similaires.\n\nDésactiver cette option reproduit le comportement de la version GameCube." } },
    { ELanguage::Spanish,  { "Confirmation Prompts", "Shows confirmation prompts when loading a save file, exiting to the menu, restarting, and in other similar situations.\n\nDisabling this option replicates the behavior of the GameCube version." } },
    { ELanguage::Italian,  { "Confirmation Prompts", "Shows confirmation prompts when loading a save file, exiting to the menu, restarting, and in other similar situations.\n\nDisabling this option replicates the behavior of the GameCube version." } },
};

CONFIG_DEFINE_LOCALE(ControllerIcons)
{
    { ELanguage::English, { "Controller Icons", "Change the icons to match your controller." } },
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
    { ELanguage::English, { "1P Vibration", "Toggle controller vibration." } },
    { ELanguage::Japanese, { "1P[振動:しんどう]", "[振動:しんどう]の\u200B[有無:うむ]を\u200B[選択:せんたく]できます" } },
    { ELanguage::German,   { "1P Vibration", "Schalte die Controllervibration an oder aus." } },
    { ELanguage::French,   { "1P Vibration", "Active les vibrations de la manette." } },
    { ELanguage::Spanish,  { "1P Vibración", "Activa o desactiva la vibración del mando." } },
    { ELanguage::Italian,  { "1P Vibrazione", "Attiva/disattiva la vibrazione del controller." } }
};

CONFIG_DEFINE_LOCALE(Vibration2P)
{
    { ELanguage::English, { "2P Vibration", "Toggle controller vibration." } },
    { ELanguage::Japanese, { "2P[振動:しんどう]", "[振動:しんどう]の\u200B[有無:うむ]を\u200B[選択:せんたく]できます" } },
    { ELanguage::German,   { "2P Vibration", "Schalte die Controllervibration an oder aus." } },
    { ELanguage::French,   { "2P Vibration", "Active les vibrations de la manette." } },
    { ELanguage::Spanish,  { "2P Vibración", "Activa o desactiva la vibración del mando." } },
    { ELanguage::Italian,  { "2P Vibrazione", "Attiva/disattiva la vibrazione del controller." } }
};

CONFIG_DEFINE_LOCALE(Vibration3P)
{
    { ELanguage::English, { "3P Vibration", "Toggle controller vibration." } },
    { ELanguage::Japanese, { "3P[振動:しんどう]", "[振動:しんどう]の\u200B[有無:うむ]を\u200B[選択:せんたく]できます" } },
    { ELanguage::German,   { "3P Vibration", "Schalte die Controllervibration an oder aus." } },
    { ELanguage::French,   { "3P Vibration", "Active les vibrations de la manette." } },
    { ELanguage::Spanish,  { "3P Vibración", "Activa o desactiva la vibración del mando." } },
    { ELanguage::Italian,  { "3P Vibrazione", "Attiva/disattiva la vibrazione del controller." } }
};

CONFIG_DEFINE_LOCALE(Vibration4P)
{
    { ELanguage::English, { "4P Vibration", "Toggle controller vibration." } },
    { ELanguage::Japanese, { "4P[振動:しんどう]", "[振動:しんどう]の\u200B[有無:うむ]を\u200B[選択:せんたく]できます" } },
    { ELanguage::German,   { "4P Vibration", "Schalte die Controllervibration an oder aus." } },
    { ELanguage::French,   { "4P Vibration", "Active les vibrations de la manette." } },
    { ELanguage::Spanish,  { "4P Vibración", "Activa o desactiva la vibración del mando." } },
    { ELanguage::Italian,  { "4P Vibrazione", "Attiva/disattiva la vibrazione del controller." } }
};

CONFIG_DEFINE_LOCALE(AllowBackgroundInput)
{
    { ELanguage::English, { "Allow Background Input", "Allow controller input whilst the game window is unfocused." } },
    { ELanguage::Japanese, { "バックグラウンド[入力:にゅうりょく]", "フォーカスされていない\u200Bゲームに\u200B[入力:にゅうりょく]できるか\u200B[選択:せんたく]できます" } },
    { ELanguage::German,   { "Erlaube Hintergrundeingaben", "Erlaube Eingaben deines Controllers auch wenn das Spielfenster nicht fokussiert ist." } },
    { ELanguage::French,   { "Manette en arrière plan", "Permet d'utiliser la manette dans le jeu lorsque qu'il n'est pas au premier plan." } },
    { ELanguage::Spanish,  { "Control en segundo plano", "Permite controlar el juego con un mando mientras la ventana esté en segundo plano." } },
    { ELanguage::Italian,  { "Input con la finestra inattiva", "Attiva/disattiva i tasti del controller mentre la finestra è inattiva." } }
};

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
    { ELanguage::English, { "Music Volume", "Adjust the volume for the music." } },
    { ELanguage::Japanese, { "BGMボリューム", "BGMボリュームの\u200B[大:おお]きさを\u200B[調整:ちょうせい]できます" } },
    { ELanguage::German,   { "Musiklautstärke", "Passe die Lautstärke der Musik an." } },
    { ELanguage::French,   { "Volume de la musique", "Réglage du volume de la musique." } },
    { ELanguage::Spanish,  { "Volumen de la música", "Ajusta el volumen de la música." } },
    { ELanguage::Italian,  { "Volume musica di sottofondo", "Regola il volume della musica di sottofondo." } }
};

CONFIG_DEFINE_LOCALE(EffectsVolume)
{
    { ELanguage::English, { "Effects Volume", "Adjust the volume for sound effects." } },
    { ELanguage::Japanese, { "SEボリューム", "SEボリュームの\u200B[大:おお]きさを\u200B[調整:ちょうせい]できます" } },
    { ELanguage::German,   { "Soundeffektlautstärke", "Passe die Lautstärke der Soundeffekte an." } },
    { ELanguage::French,   { "Volume des effets sonores", "Réglage du volume des effets sonores." } },
    { ELanguage::Spanish,  { "Volumen de efectos", "Ajusta el volumen de los efectos de sonido." } },
    { ELanguage::Italian,  { "Volume effetti sonori", "Regola il volume degli effetti sonori." } }
};

CONFIG_DEFINE_LOCALE(VoicesVolume)
{
    { ELanguage::English, { "Voices Volume", "Adjust the volume for the voices." } },
    { ELanguage::Japanese, { "ボイスボリューム", "ボイスボリュームの\u200B[大:おお]きさを\u200B[調整:ちょうせい]できます" } },
    { ELanguage::German,   { "Stimmenlautstärke", "Passe die Lautstärke der Stimmen an." } },
    { ELanguage::French,   { "Volume des voix", "Réglage du volume des voix." } },
    { ELanguage::Spanish,  { "Volumen de voces", "Ajusta el volumen de las voces." } },
    { ELanguage::Italian,  { "Volume delle voci", "Regola il volume delle voci." } },
};

CONFIG_DEFINE_LOCALE(MuteOnFocusLost)
{
    { ELanguage::English, { "Mute on Focus Lost", "Mute the game's audio when the window is not in focus." } },
    { ELanguage::Japanese, { "フォーカスロストミュート", "ウィンドウがフォーカスされていないときにゲームのオーディオをミュートします" } },
    { ELanguage::German,   { "Stummstellen wenn nicht fokussiert", "Stellt das Audio des Spiels stumm solange das Fenster nicht im Fokus ist." } },
    { ELanguage::French,   { "Muet si fenêtre inactive", "Coupe le son du jeu lorsque la fenêtre n'est pas active." } },
    { ELanguage::Spanish,  { "Silenciar al perder el foco", "Silencia el audio del juego cuando la ventana no esté en primer plano." } },
    { ELanguage::Italian,  { "Audio muto con finestra inattiva", "Disattiva l'audio del gioco quando la finestra non è attiva." } }
};

CONFIG_DEFINE_LOCALE(IdleChaosPowersMusic)
{
    { ELanguage::English, { "Enable Idle Chaos Powers Music", "Enable the music that plays while Hero or Dark gauge is full." } },
    { ELanguage::Japanese, { "TODO", "TODO" } },
    { ELanguage::German,   { "TODO", "TODO" } },
    { ELanguage::French,   { "Active la musique Chaos Powers passifs", "Active la musique qui joue lorsque la jauge Hero ou Dark est pleine." } },
    { ELanguage::Spanish,  { "TODO", "TODO" } },
    { ELanguage::Italian,  { "TODO", "TODO" } }
};

CONFIG_DEFINE_LOCALE(ChaosPowerLEDSync)
{
    { ELanguage::English, { "Chaos Power LED Sync", "Sync the controller's LED color with the active Chaos Power." } },
    { ELanguage::Japanese, { "TODO", "TODO" } },
    { ELanguage::German,   { "TODO", "TODO" } },
    { ELanguage::French,   { "Chaos Power LED Sync", "Synchronise la couleur des LED de la manette avec le Chaos Power actif." } },
    { ELanguage::Spanish,  { "TODO", "TODO" } },
    { ELanguage::Italian,  { "TODO", "TODO" } }
};

CONFIG_DEFINE_LOCALE(VoiceLanguage)
{
    { ELanguage::English, { "Voice Language", "Change the language used for character voices." } },
    { ELanguage::Japanese, { "[音声言語:おんせいげんご]", "ゲーム[内:ない]の\u200B[音声言語:おんせいげんご]を\u200B[変更:へんこう]できます" } },
    { ELanguage::German,   { "Stimmeinstellung", "Ändere die Sprache, die für Charakterstimmen benutzt wird." } },
    { ELanguage::French,   { "Langue des voix", "Modifie la langue utilisée pour la voix des personnages." } },
    { ELanguage::Spanish,  { "Idioma de voz", "Cambia el idioma utilizado para las voces de los personajes." } },
    { ELanguage::Italian, { "Lingua delle voci", "Modifica la lingua utilizzata per le voci dei personaggi." } }
};

CONFIG_DEFINE_LOCALE(Subtitles)
{
    { ELanguage::English, { "Subtitles", "Show subtitles during dialogue." } },
    { ELanguage::Japanese, { "[字幕:じまく]", "[字幕:じまく]の[表示:ひょうじ]を\u200B[選択:せんたく]できます" } },
    { ELanguage::German,   { "Untertitel", "Zeige Untertitel bei Dialogen." } },
    { ELanguage::French,   { "Sous-titres", "Affiche les sous-titres pendant les dialogues." } },
    { ELanguage::Spanish,  { "Subtítulos", "Muestra subtítulos durante los diálogos." } },
    { ELanguage::Italian,  { "Sottotitoli", "Mostra i sottotitoli durante i dialoghi." } }
};

CONFIG_DEFINE_LOCALE(WindowSize)
{
    { ELanguage::English, { "Resolution", "Adjust the resolution of the game in windowed mode." } },
    { ELanguage::Japanese, { "解像度", "ウィンドウ\u200Bモードでの\u200Bゲームの\u200B[解像度:かいぞうど]を\u200B[調整:ちょうせい]できます" } },
    { ELanguage::German,   { "Auflösung", "Ändere die Auflösung des Spiels im Fenstermodus." } },
    { ELanguage::French,   { "Résolution", "Modifie la résolution du jeu en mode fenêtré." } },
    { ELanguage::Spanish,  { "Resolución", "Ajusta la resolución del juego en modo ventana." } },
    { ELanguage::Italian,  { "Risoluzione", "Regola la risoluzione del gioco in modalità finestra." } }
};

CONFIG_DEFINE_LOCALE(Monitor)
{
    { ELanguage::English, { "Monitor", "Change which monitor to display the game on." } },
    { ELanguage::Japanese, { "モニター[選択:せんたく]", "ゲームを[表示:ひょうじ]する\u200Bモニターを\u200B[変更:へんこう]できます" } },
    { ELanguage::German,   { "Monitor", "Ändere auf welchem Monitor das Spiel angezeigt wird." } },
    { ELanguage::French,   { "Moniteur", "Change le moniteur sur lequel le jeu sera affiché." } },
    { ELanguage::Spanish,  { "Pantalla", "Cambia la pantalla en la cuál se muestra el juego." } },
    { ELanguage::Italian,  { "Schermo", "Cambia lo schermo su cui visualizzare il gioco." } }
};

CONFIG_DEFINE_LOCALE(AspectRatio)
{
    { ELanguage::English, { "Aspect Ratio", "Change the aspect ratio." } },
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
    { ELanguage::English,  { "FMV Aspect Ratio", "Change how full motion videos fit the display." } },
    { ELanguage::Japanese, { "FMV Aspect Ratio", "Change how full motion videos fit the display." } },
    { ELanguage::German,   { "FMV Aspect Ratio", "Change how full motion videos fit the display." } },
    { ELanguage::French,   { "FMV Aspect Ratio", "Change how full motion videos fit the display." } },
    { ELanguage::Spanish,  { "FMV Aspect Ratio", "Change how full motion videos fit the display." } },
    { ELanguage::Italian,  { "FMV Aspect Ratio", "Change how full motion videos fit the display." } }
};

// Japanese Notes: This localization should include furigana in its description.
CONFIG_DEFINE_ENUM_LOCALE(EFMVAspectRatio)
{
    {
        ELanguage::English,
        {
            { EFMVAspectRatio::Stretched, { "STRETCHED", "Stretched: the video is stretched to fill the display, as the original game does." } },
            { EFMVAspectRatio::Fit,       { "FIT", "Fit: the video keeps its original shape and black bars fill the rest of the display." } },
            { EFMVAspectRatio::Fill,      { "FILL", "Fill: the video keeps its original shape and is cropped to fill the display." } }
        }
    },
    {
        ELanguage::Japanese,
        {
            { EFMVAspectRatio::Stretched, { "STRETCHED", "Stretched: the video is stretched to fill the display, as the original game does." } },
            { EFMVAspectRatio::Fit,       { "FIT", "Fit: the video keeps its original shape and black bars fill the rest of the display." } },
            { EFMVAspectRatio::Fill,      { "FILL", "Fill: the video keeps its original shape and is cropped to fill the display." } }
        }
    },
    {
        ELanguage::German,
        {
            { EFMVAspectRatio::Stretched, { "STRETCHED", "Stretched: the video is stretched to fill the display, as the original game does." } },
            { EFMVAspectRatio::Fit,       { "FIT", "Fit: the video keeps its original shape and black bars fill the rest of the display." } },
            { EFMVAspectRatio::Fill,      { "FILL", "Fill: the video keeps its original shape and is cropped to fill the display." } }
        }
    },
    {
        ELanguage::French,
        {
            { EFMVAspectRatio::Stretched, { "STRETCHED", "Stretched: the video is stretched to fill the display, as the original game does." } },
            { EFMVAspectRatio::Fit,       { "FIT", "Fit: the video keeps its original shape and black bars fill the rest of the display." } },
            { EFMVAspectRatio::Fill,      { "FILL", "Fill: the video keeps its original shape and is cropped to fill the display." } }
        }
    },
    {
        ELanguage::Spanish,
        {
            { EFMVAspectRatio::Stretched, { "STRETCHED", "Stretched: the video is stretched to fill the display, as the original game does." } },
            { EFMVAspectRatio::Fit,       { "FIT", "Fit: the video keeps its original shape and black bars fill the rest of the display." } },
            { EFMVAspectRatio::Fill,      { "FILL", "Fill: the video keeps its original shape and is cropped to fill the display." } }
        }
    },
    {
        ELanguage::Italian,
        {
            { EFMVAspectRatio::Stretched, { "STRETCHED", "Stretched: the video is stretched to fill the display, as the original game does." } },
            { EFMVAspectRatio::Fit,       { "FIT", "Fit: the video keeps its original shape and black bars fill the rest of the display." } },
            { EFMVAspectRatio::Fill,      { "FILL", "Fill: the video keeps its original shape and is cropped to fill the display." } }
        }
    }
};

CONFIG_DEFINE_LOCALE(ResolutionScale)
{
    { ELanguage::English, { "Resolution Scale", "Adjust the internal resolution of the game.\n\n" } },
    { ELanguage::Japanese, { "[解像度:かいぞうど]スケール", "ゲームの\u200B[内部解像度:ないぶかいぞうど]を\u200B[調整:ちょうせい]できます\n\n" } },
    { ELanguage::German,   { "Rendering-Auflösung", "Passe die Auflösung der internen Darstellung an.\n\n" } },
    { ELanguage::French,   { "Échelle de rendu", "Modifie la résolution interne du jeu.\n\n" } },
    { ELanguage::Spanish,  { "Escala de resolución", "Ajusta la resolución interna del juego.\n\n" } },
    { ELanguage::Italian, { "Scala risoluzione", "Regola la risoluzione interna del gioco.\n\n" } }
};

CONFIG_DEFINE_LOCALE(Fullscreen)
{
    { ELanguage::English, { "Fullscreen", "Toggle between borderless fullscreen or windowed mode." } },
    { ELanguage::Japanese, { "フルスクリーン", "ボーダーレス\u200Bフルスクリーンか\u200Bウィンドウモードを\u200B[選択:せんたく]できます" } },
    { ELanguage::German,   { "Vollbild", "Wechsle zwischen dem randlosen Vollbildmodus und dem Fenstermodus." } },
    { ELanguage::French,   { "Plein écran", "Alterne entre le mode plein écran sans bordures et le mode fenêtré." } },
    { ELanguage::Spanish,  { "Pantalla completa", "Cambia entre modo de pantalla completa o ventana." } },
    { ELanguage::Italian,  { "Schermo pieno", "Attiva/disattiva tra modalità finestra senza cornice e modalità finestra." } }
};

CONFIG_DEFINE_LOCALE(VSync)
{
    { ELanguage::English, { "V-Sync", "Synchronize the game to the refresh rate of the display to prevent screen tearing." } },
    { ELanguage::Japanese, { "[垂直同期:すいちょくどうき]", "[垂直同期:すいちょくどうき]の\u200B[設定:せってい]を\u200B[変更:へんこう]できます" } },
    { ELanguage::German,   { "V-Sync", "Synchronisiere das Spiel mit der Bildwiederholrate deines Bildschirms um Bildverzerrungen zu vermeiden." } },
    { ELanguage::French,   { "V-Sync", "Synchronise le jeu avec la fréquence de rafraîchissement de l'écran pour éviter le screen tearing." } },
    { ELanguage::Spanish,  { "V-Sync", "Sincroniza el juego a la tasa de refresco de la pantalla para prevenir el rasgado de la imagen." } },
    { ELanguage::Italian,  { "V-Sync", "Sincronizza il gioco con la frequenza d'aggiornamento del display per evitare lo screen tearing." } }
};

CONFIG_DEFINE_LOCALE(FPS)
{
    { ELanguage::English, { "FPS", "Set the max frame rate the game can run at.\n\nWARNING: this may introduce glitches at frame rates higher than 60 FPS." } },
    { ELanguage::Japanese, { "フレームレート[上限:じょうげん]", "ゲームの\u200B[最大:さいだい]フレームレートを\u200B[設定:せってい]できます\n\n[警告:けいこく]: 60 FPSを\u200B[超:こ]えるフレームレートで\u200B[不具合:ふぐあい]が\u200B[発生:はっせい]する\u200B[可能性:かのうせい]が\u200Bあります" } },
    { ELanguage::German,   { "FPS", "Setze die maximale Anzahl der Bilder pro Sekunde, die das Spiel darstellen kann.\n\nWARNUNG: Das Spiel kann bei höheren FPS als 60 ungewolltes Verhalten aufweisen." } },
    { ELanguage::French,   { "IPS", "Détermine la fréquence d'images maximale du jeu.\n\nATTENTION : cela peut entraîner des problèmes à des taux de rafraîchissement supérieurs à 60 IPS." } },
    { ELanguage::Spanish,  { "FPS", "Establece la tasa de fotogramas máxima a la que puede correr el juego.\n\nADVERTENCIA: esto puede introducir fallos en tasas mayores a 60 FPS." } },
    { ELanguage::Italian,  { "FPS", "Imposta il frame rate massimo del gioco.\n\nATTENZIONE: questa opzione può causare dei glitch a frame rate più alti di 60 FPS." } }
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

CONFIG_DEFINE_LOCALE(AntiAliasing)
{
    { ELanguage::English, { "Anti-Aliasing", "Adjust the amount of smoothing applied to jagged edges." } },
    { ELanguage::Japanese, { "アンチエイリアス", "アンチエイリアスの\u200B[種類:しゅるい]を\u200B[選択:せんたく]できます" } },
    { ELanguage::German,   { "Kantenglättung", "Passe die Menge an Kantenglättung an." } },
    { ELanguage::French,   { "Anticrénelage", "Ajuste le niveau d'anticrénelage appliqué aux bords des objets." } },
    { ELanguage::Spanish,  { "Anti-Aliasing", "Ajusta el nivel de suavizado aplicado a los dientes de sierra." } },
    { ELanguage::Italian,  { "Anti-Aliasing", "Regola la quantità di smussamento applicata ai bordi." } }
};

CONFIG_DEFINE_ENUM_LOCALE(EAntiAliasing)
{
    {
        ELanguage::English,
        {
            { EAntiAliasing::None, { "NONE", "" } }
        }
    },
    {
        ELanguage::Japanese,
        {
            { EAntiAliasing::None, { "オフ", "" } },
        }
    },
    {
        ELanguage::German,
        {
            { EAntiAliasing::None, { "KEINE", "" } }
        }
    },
    {
        ELanguage::French,
        {
            { EAntiAliasing::None, { "SANS", "" } }
        }
    },
    {
        ELanguage::Spanish,
        {
            { EAntiAliasing::None, { "NO", "" } },
        }
    },
    {
        ELanguage::Italian,
        {
            { EAntiAliasing::None, { "NULLO", "" } }
        }
    },
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
    { ELanguage::Japanese, { "Bloom Intensity", "" } },
    { ELanguage::German,   { "Bloom-Intensität", "Passe die Intensität des Bloom-Effekts an." } },
    { ELanguage::French,   { "Intensité du bloom", "Réglez l'intensité de l'effet du bloom." } },
    { ELanguage::Spanish,  { "Intensidad del bloom", "Ajusta la intensidad del efecto de bloom." } },
    { ELanguage::Italian,  { "Intensità del bloom", "Regola l'intensità dell'effetto bloom." } }
};

CONFIG_DEFINE_LOCALE(EnablePartnerIntro)
{
    { ELanguage::English, { "Partner Intro", "Show partner's introduction when they are first met." } },
    { ELanguage::Japanese, { "", "" } },
    { ELanguage::German,   { "", "" } },
    { ELanguage::French,   { "Intro du Partenaire", "Afficher l'introduction du partenaire lorsqu'il est rencontré pour la première fois." } },
    { ELanguage::Spanish,  { "", "" } },
    { ELanguage::Italian,  { "", "" } }
};

CONFIG_DEFINE_LOCALE(UIAlignmentMode)
{
    { ELanguage::English, { "UI Alignment Mode", "Change how the UI aligns with the display." } },
    { ELanguage::Japanese, { "UIアライメントモード", "UIとディスプレイの\u200B[配置:はいち]を\u200B[変更:へんこう]できます" } },
    { ELanguage::German,   { "Benutzeroberflächenausrichtung", "Verändere wie die Benutzeroberfläche sich mit dem Bildschirm ausrichtet." } },
    { ELanguage::French,   { "Alignement de l'IU", "Modifie l'alignement de l'interface utilisateur sur l'écran." } },
    { ELanguage::Spanish,  { "Modo de alineamiento de UI", "Cambia la alineación de la interfaz de usuario con la pantalla." } },
    { ELanguage::Italian,  { "Modalità allineamento interfaccia", "Modifica come l'interfaccia si allinea con lo schermo." } }
};

// Japanese Notes: This localization should include furigana in its description.
CONFIG_DEFINE_ENUM_LOCALE(EUIAlignmentMode)
{
    {
        ELanguage::English,
        {
            { EUIAlignmentMode::Edge,    { "EDGE", "Edge: the UI will align with the edges of the display." } },
            { EUIAlignmentMode::Centre,  { "CENTER", "Center: the UI will align with the center of the display." } },
            { EUIAlignmentMode::Stretch, { "STRETCH", "Stretch: the UI is stretched to fill the display, as the original game does." } }
        }
    },
    {
        ELanguage::Japanese,
        {
            { EUIAlignmentMode::Edge,    { "エッジ", "エッジ: UIがディスプレイの\u200B[端:はし]に\u200B[揃:そろ]います" } },
            { EUIAlignmentMode::Centre,  { "センター", "センター: UIがディスプレイの\u200B[中央:ちゅうおう]に\u200B[揃:そろ]います" } },
                       { EUIAlignmentMode::Stretch, { "STRETCH", "Stretch: the UI is stretched to fill the display, as the original game does." } }
        }
    },
    {
        ELanguage::German,
        {
            { EUIAlignmentMode::Edge,    { "RAND", "Rand: Die Benutzeroberfläche richtet sich zum Rand des Bildschirms aus." } },
            { EUIAlignmentMode::Centre,  { "MITTE", "Mitte: Die Benutzeroberfläche richtet sich zur Mitte des Bildschirms aus." } },
            { EUIAlignmentMode::Stretch, { "STRETCH", "Stretch: the UI is stretched to fill the display, as the original game does." } }
        }
    },
    {
        ELanguage::French,
        {
            { EUIAlignmentMode::Edge,    { "BORD", "Bord : l'interface utilisateur sera alignée sur les bords de l'écran." } },
            { EUIAlignmentMode::Centre,  { "CENTRÉE", "Centrée : l'interface utilisateur sera alignée sur le centre de l'écran." } },
            { EUIAlignmentMode::Stretch, { "ÉTIRÉE", "Étirée : l'interface utilisateur est étirée sur toute la largeur de l'écran, comme dans le jeu d'origine." } },
        }
    },
    {
        ELanguage::Spanish,
        {
            { EUIAlignmentMode::Edge,    { "BORDE", "Borde: la interfaz de usuario se alineará con los bordes de la pantalla." } },
            { EUIAlignmentMode::Centre,  { "CENTRO", "Centro: la interfaz de usuario se alineará con el centro de la pantalla." } },
            { EUIAlignmentMode::Stretch, { "STRETCH", "Stretch: the UI is stretched to fill the display, as the original game does." } }
        }
    },
    {
        ELanguage::Italian,
        {
            { EUIAlignmentMode::Edge,    { "BORDI", "Bordi: l'interfaccia si allineerà con i bordi dello schermo." } },
            { EUIAlignmentMode::Centre,  { "CENTRO", "Centro: l'interfaccia si allineerà con il centro dello schermo." } },
            { EUIAlignmentMode::Stretch, { "STRETCH", "Stretch: the UI is stretched to fill the display, as the original game does." } }
        }
    }
};