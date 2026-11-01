#include <user/config.h>
#include <locale/locale.h>

// Japanese localization should use furigana for message windows, options items, options descriptions and installer text.
std::unordered_map<std::string_view, std::unordered_map<ELanguage, std::string>> g_locale =
{
	{
		"Options_Category_System",
		{
			{ ELanguage::English,  "SYSTEM" },
			{ ELanguage::Japanese, "SYSTEM" },
			{ ELanguage::German,   "SYSTEM" },
			{ ELanguage::French,   "SYSTÈME" },
			{ ELanguage::Spanish,  "SISTEMA" },
			{ ELanguage::Italian,  "SISTEMA" }
		}
	},
	{
		"Options_Desc_Category_System",
		{
			{ ELanguage::English,  "Adjust system settings." },
			{ ELanguage::Japanese, "システムの設定を変更します" },
			{ ELanguage::German,   "Verändere System-Einstellungen." },
			{ ELanguage::French,   "Modifier les paramètres système." },
			{ ELanguage::Spanish,  "Ajustar la configuración del sistema." },
			{ ELanguage::Italian,  "Modifica le impostazioni di sistema." }
		}
	},
	{
		"Options_Category_Input",
		{
			{ ELanguage::English,  "INPUT" },
			{ ELanguage::Japanese, "INPUT" },
			{ ELanguage::German,   "EINGABE" },
			{ ELanguage::French,   "COMMANDES" },
			{ ELanguage::Spanish,  "CONTROLES" },
			{ ELanguage::Italian,  "COMANDI" }
		}
	},
	{
		"Options_Desc_Category_Input",
		{
			{ ELanguage::English,  "Adjust input settings." },
			{ ELanguage::Japanese, "入力の設定を変更します" },
			{ ELanguage::German,   "Verändere Eingabe-Einstellungen." },
			{ ELanguage::French,   "Modifier les paramètres d'entrée." },
			{ ELanguage::Spanish,  "Ajustar la configuración de entrada." },
			{ ELanguage::Italian,  "Modifica le impostazioni input." }
		}
	},
	{
		"Options_Category_Audio",
		{
			{ ELanguage::English,  "AUDIO" },
			{ ELanguage::Japanese, "AUDIO" },
			{ ELanguage::German,   "AUDIO" },
			{ ELanguage::French,   "SON" },
			{ ELanguage::Spanish,  "AUDIO" },
			{ ELanguage::Italian,  "AUDIO" }
		}
	},
	{
		"Options_Desc_Category_Audio",
		{
			{ ELanguage::English,  "Adjust audio settings." },
			{ ELanguage::Japanese, "オーディオの設定を変更します" },
			{ ELanguage::German,   "Verändere Audio-Einstellungen." },
			{ ELanguage::French,   "Modifier les paramètres audio." },
			{ ELanguage::Spanish,  "Ajustar la configuración de audio." },
			{ ELanguage::Italian,  "Modifica le impostazioni audio." }
		}
	},
	{
		"Options_Category_Video",
		{
			{ ELanguage::English,  "VIDEO" },
			{ ELanguage::Japanese, "VIDEO" },
			{ ELanguage::German,   "VIDEO" },
			{ ELanguage::French,   "AFFICHAGE" },
			{ ELanguage::Spanish,  "VÍDEO" },
			{ ELanguage::Italian,  "VIDEO" }
		}
	},
	{
		"Options_Desc_Category_Video",
		{
			{ ELanguage::English,  "Adjust video settings." },
			{ ELanguage::Japanese, "ビデオの設定を変更します" },
			{ ELanguage::German,   "Verändere Video-Einstellungen." },
			{ ELanguage::French,   "Modifier les paramètres vidéo." },
			{ ELanguage::Spanish,  "Ajustar la configuración de vídeo." },
			{ ELanguage::Italian,  "Modifica le impostazioni video." }
		}
	},
	{
		// Notes: integer values in the options menu (e.g. FPS) when at their maximum value.
		"Options_Value_Max",
		{
			{ ELanguage::English,  "MAX" },
			{ ELanguage::Japanese, "MAX" },
			{ ELanguage::German,   "MAX" },
			{ ELanguage::French,   "MAX" },
			{ ELanguage::Spanish,  "MÁX" },
			{ ELanguage::Italian,  "MAX" }
		}
	},
	{
		"Options_Value_Custom",
		{
			{ ELanguage::English,  "CUSTOM" },
			{ ELanguage::Japanese, "CUSTOM" },
			{ ELanguage::German,   "CUSTOM" },
			{ ELanguage::French,   "CUSTOM" },
			{ ELanguage::Spanish,  "CUSTOM" },
			{ ELanguage::Italian,  "CUSTOM" }
		}
	},
	{
		"Options_Value_AllScreens",
		{
			{ ELanguage::English,  "ALL SCREENS" },
			{ ELanguage::Japanese, "ALL SCREENS" },
			{ ELanguage::German,   "ALL SCREENS" },
			{ ELanguage::French,   "TOUS LES ÉCRANS" },
			{ ELanguage::Spanish,  "ALL SCREENS" },
			{ ELanguage::Italian,  "ALL SCREENS" },
		}
	},
   {
	   // Notes: description for Music Attenuation when the user is not running a supported OS.
	   // Japanese Notes: This localization should include furigana.
	   "Options_Desc_OSNotSupported",
	   {
		   { ELanguage::English,  "This option is not supported by your operating system." },
		   { ELanguage::Japanese, "この\u200Bオプションは\u200B[現在:げんざい]の\u200BOSで\u200B[変更:へんこう]\u200Bできません" },
		   { ELanguage::German,   "Diese Option wird von diesem Betriebssystem nicht unterstützt." },
		   { ELanguage::French,   "Cette option n'est pas prise en charge par votre système d'exploitation." },
		   { ELanguage::Spanish,  "Esta opción no está soportada por tu sistema operativo." },
		   { ELanguage::Italian,  "Questa opzione non è disponibile con il tuo sistema operativo." }
	   }
},
{
	"Options_Name_CustomizeInput",
	{
		{ ELanguage::English,  "Customize Input" },
		{ ELanguage::Japanese, "Customize Input" },
		{ ELanguage::German,   "Customize Input" },
		{ ELanguage::French,   "Customize Input" },
		{ ELanguage::Spanish,  "Customize Input" },
		{ ELanguage::Italian,  "Customize Input" }
	}
},
{
	"Options_Desc_CustomizeInput",
	{
		{ ELanguage::English,  "Customize controls for each player." },
		{ ELanguage::Japanese, "Customize controls for each player." },
		{ ELanguage::German,   "Customize controls for each player." },
		{ ELanguage::French,   "Customize controls for each player." },
		{ ELanguage::Spanish,  "Customize controls for each player." },
		{ ELanguage::Italian,  "Customize controls for each player." }
	}
},
{
	"Options_Category_Keyboard",
	{
		{ ELanguage::English,  "KEYBOARD" },
		{ ELanguage::Japanese, "KEYBOARD" },
		{ ELanguage::German,   "KEYBOARD" },
		{ ELanguage::French,   "KEYBOARD" },
		{ ELanguage::Spanish,  "KEYBOARD" },
		{ ELanguage::Italian,  "KEYBOARD" }
	}
},
{
	"Options_Desc_Category_Keyboard",
	{
		{ ELanguage::English,  "Customize keyboard controls." },
		{ ELanguage::Japanese, "Customize keyboard controls." },
		{ ELanguage::German,   "Customize keyboard controls." },
		{ ELanguage::French,   "Customize keyboard controls." },
		{ ELanguage::Spanish,  "Customize keyboard controls." },
		{ ELanguage::Italian,  "Customize keyboard controls." }
	}
},
{
	// Notes: {} is the player number (1P, 2P, 3P, 4P)
	"Options_Category_Gamepad",
	{
		{ ELanguage::English,  "{}P GAMEPAD" },
		{ ELanguage::Japanese, "{}P GAMEPAD" },
		{ ELanguage::German,   "{}P GAMEPAD" },
		{ ELanguage::French,   "{}P GAMEPAD" },
		{ ELanguage::Spanish,  "{}P GAMEPAD" },
		{ ELanguage::Italian,  "{}P GAMEPAD" }
	}
},
{
	"Options_Desc_Category_Gamepad",
	{
		{ ELanguage::English,  "Customize {}P gamepad buttons." },
		{ ELanguage::Japanese, "Customize {}P gamepad buttons." },
		{ ELanguage::German,   "Customize {}P gamepad buttons." },
		{ ELanguage::French,   "Customize {}P gamepad buttons." },
		{ ELanguage::Spanish,  "Customize {}P gamepad buttons." },
		{ ELanguage::Italian,  "Customize {}P gamepad buttons." }
	}
},
{
	"Options_Desc_BindKey",
	{
		{ ELanguage::English,  "Change key assignment." },
		{ ELanguage::Japanese, "Change key assignment." },
		{ ELanguage::German,   "Change key assignment." },
		{ ELanguage::French,   "Change key assignment." },
		{ ELanguage::Spanish,  "Change key assignment." },
		{ ELanguage::Italian,  "Change key assignment." }
	}
},
{
	"Options_Desc_BindButton",
	{
		{ ELanguage::English,  "Change assigned button." },
		{ ELanguage::Japanese, "Change assigned button." },
		{ ELanguage::German,   "Change assigned button." },
		{ ELanguage::French,   "Change assigned button." },
		{ ELanguage::Spanish,  "Change assigned button." },
		{ ELanguage::Italian,  "Change assigned button." }
	}
},
{
	"Options_Desc_BindKeyListening",
	{
		{ ELanguage::English,  "Press key to assign. Press Escape or wait to cancel." },
		{ ELanguage::Japanese, "Press key to assign. Press Escape or wait to cancel." },
		{ ELanguage::German,   "Press key to assign. Press Escape or wait to cancel." },
		{ ELanguage::French,   "Press key to assign. Press Escape or wait to cancel." },
		{ ELanguage::Spanish,  "Press key to assign. Press Escape or wait to cancel." },
		{ ELanguage::Italian,  "Press key to assign. Press Escape or wait to cancel." }
	}
},
{
	"Options_Desc_BindButtonListening",
	{
		{ ELanguage::English,  "Press button to assign. Press Escape or wait to cancel." },
		{ ELanguage::Japanese, "Press button to assign. Press Escape or wait to cancel." },
		{ ELanguage::German,   "Press button to assign. Press Escape or wait to cancel." },
		{ ELanguage::French,   "Press button to assign. Press Escape or wait to cancel." },
		{ ELanguage::Spanish,  "Press button to assign. Press Escape or wait to cancel." },
		{ ELanguage::Italian,  "Press button to assign. Press Escape or wait to cancel." }
	}
},
{
	"Options_Name_SoundTest",
	{
		{ ELanguage::English,  "Sound Test" },
		{ ELanguage::Japanese, "Sound Test" },
		{ ELanguage::German,   "Sound Test" },
		{ ELanguage::French,   "Test Audio" },
		{ ELanguage::Spanish,  "Sound Test" },
		{ ELanguage::Italian,  "Sound Test" }
	}
},
{
	"Options_Desc_SoundTest",
	{
		{ ELanguage::English,  "Listen to the in-game music." },
		{ ELanguage::Japanese, "Listen to the in-game music." },
		{ ELanguage::German,   "Höre die Musik des Spiels an." },
		{ ELanguage::French,   "Écoutez la musique du jeu." },
		{ ELanguage::Spanish,  "Escuchar la música del juego." },
		{ ELanguage::Italian,  "Ascolta la musica del gioco." }
	}
},
{
	"Options_Desc_SoundTest_Track",
	{
		{ ELanguage::English,  "Select a track to play it." },
		{ ELanguage::Japanese, "Select a track to play it" },
		{ ELanguage::German,   "Select a track to play it." },
		{ ELanguage::French,   "Sélectionnez une musique pour la jouer." },
		{ ELanguage::Spanish,  "Select a track to play it." },
		{ ELanguage::Italian,  "Select a track to play it." },
	}
},
{
	// Notes: shown next to the sound test track that is currently playing.
	"Options_Value_Playing",
	{
		{ ELanguage::English,  "Playing" },
		{ ELanguage::Japanese, "再生中" },
		{ ELanguage::German,   "Läuft" },
		{ ELanguage::French,   "Lecture" },
		{ ELanguage::Spanish,  "Sonando" },
		{ ELanguage::Italian,  "In riproduzione" }
	}
},
{
	// Notes: description for options that cannot be accessed anywhere but the title screen or world map (e.g. Language).
	"Options_Desc_NotAvailable",
	{
		{ ELanguage::English,  "This option is not available at this location." },
		{ ELanguage::Japanese, "この\u200Bオプションは\u200B[現在:げんざい]の\u200B[画面:がめん]では\u200B[変更:へんこう]\u200Bできません" },
		{ ELanguage::German,   "Diese Option ist an dieser Stelle nicht verfügbar." },
		{ ELanguage::French,   "Cette option n'est pas disponible pour l'instant." },
		{ ELanguage::Spanish,  "Esta opción no está disponible en este momento." },
		{ ELanguage::Italian,  "Questa opzione non può essere modificata in questo momento." }
	}
},
{
	// Notes: currently the description for Monitor when in fullscreen.
	"Options_Desc_NotAvailableWindowed",
	{
		{ ELanguage::English,  "This option is not available in windowed mode." },
		{ ELanguage::Japanese, "この\u200Bオプションは\u200Bウィンドウモードでは\u200B[変更:へんこう]\u200Bできません" },
		{ ELanguage::German,   "Diese Option ist im Fenstermodus nicht verfügbar." },
		{ ELanguage::French,   "Cette option n'est pas disponible en mode fenêtré." },
		{ ELanguage::Spanish,  "Esta opción no está disponible en modo ventana." },
		{ ELanguage::Italian,  "Questa opzione non è disponibile in modalità finestra." }
	}
},
{
	// Notes: currently the description for Monitor when the user only has one display connected.
	"Options_Desc_NotAvailableHardware",
	{
		{ ELanguage::English,  "This option is not available with your current hardware configuration." },
		{ ELanguage::Japanese, "この\u200Bオプションは\u200B[現在:げんざい]の\u200Bハードウェア[構成:こうせい]で\u200B[変更:へんこう]\u200Bできません" },
		{ ELanguage::German,   "Diese Option ist mit der momentanen Hardwarekonfiguration nicht verfügbar." },
		{ ELanguage::French,   "Cette option n'est pas disponible avec votre configuration matérielle actuelle." },
		{ ELanguage::Spanish,  "Esta opción no está disponible con tu configuración actual de hardware." },
		{ ELanguage::Italian,  "Questa opzione non è disponibile con l'hardware in tuo possesso." }
	}
},
{
	// Notes: message appears when changing the Language option and backing out of the options menu.
	"Options_Message_Restart",
	{
		{ ELanguage::English,  "The game will now restart." },
		{ ELanguage::Japanese, "ゲームが[再起動:さいきどう]されます" },
		{ ELanguage::German,   "Das Spiel wird jetzt neu starten." },
		{ ELanguage::French,   "Le jeu va maintenant redémarrer." },
		{ ELanguage::Spanish,  "El juego se va a reiniciar." },
		{ ELanguage::Italian,  "Il gioco verrà riavviato." }
	}
},
{
	// Notes: text in pause menu during gameplay
	"Pause_Menu_Continue",
	{
		{ ELanguage::English,  "resume" },
		{ ELanguage::Japanese, "resume" },
		{ ELanguage::German,   "weiter" },
		{ ELanguage::French,   "reprendre" },
		{ ELanguage::Spanish,  "continuar" },
		{ ELanguage::Italian,  "continua" }
	}
},
{
	// Notes: text in pause menu during gameplay
	// Font doesn't support accent so the typo in french is intended
	"Pause_Menu_Restart",
	{
		{ ELanguage::English,  "restart" },
		{ ELanguage::Japanese, "restart" },
		{ ELanguage::German,   "neustart" },
		{ ELanguage::French,   "redémarrer" },
		{ ELanguage::Spanish,  "volver a empezar" },
		{ ELanguage::Italian,  "ricomincia" }
	}
},
{
	// Notes: text in pause menu during gameplay
	"Pause_Menu_Options",
	{
		{ ELanguage::English,  "options" },
		{ ELanguage::Japanese, "オプション" },
		{ ELanguage::German,   "optionen" },
		{ ELanguage::French,   "options" },
		{ ELanguage::Spanish,  "opciones" },
		{ ELanguage::Italian,  "opzioni" }
	}
},
{
	// Notes: text in pause menu during gameplay
	"Pause_Menu_Quit",
	{
		{ ELanguage::English,  "quit" },
		{ ELanguage::Japanese, "やめる" },
		{ ELanguage::German,   "beenden" },
		{ ELanguage::French,   "quitter" },
		{ ELanguage::Spanish,  "abandonar" },
		{ ELanguage::Italian,  "abbandona" }
	}
},
{
	// Notes: used for the button guide at the pause menu.
	// This font doesn't support accent so the typo in french is intended.
	"Achievements_Name",
	{
		{ ELanguage::English,  "achievements" },
		{ ELanguage::Japanese, "実績" },
		{ ELanguage::German,   "erfolge" },
		{ ELanguage::French,   "succès" },
		{ ELanguage::Spanish,  "logros" },
		{ ELanguage::Italian,  "obiettivi" }
	}
},
{
	"Achievements_Unlock",
	{
		{ ELanguage::English,  "Achievement Unlocked!" },
		{ ELanguage::Japanese, "実績のロックが解除されました" },
		{ ELanguage::German,   "Erfolg Freigeschaltet!" },
		{ ELanguage::French,   "Succès déverrouillé !" },
		{ ELanguage::Spanish,  "¡Logro desbloqueado!" },
		{ ELanguage::Italian,  "Obiettivo sbloccato!" }
	}
},
{
	"Installer_Header_Browse",
	{
		{ ELanguage::English,  "Browse..." },
		{ ELanguage::Japanese, "ブラウズ" },
		{ ELanguage::German,   "Durchsuchen..." },
		{ ELanguage::French,   "Parcourir..." },
		{ ELanguage::Spanish,  "Explorar..." },
		{ ELanguage::Italian,  "Esplora..." },
	}
},
{
	"Installer_Page_SelectLanguage",
	{
		{ ELanguage::English,  "Please select a language." },
		{ ELanguage::Japanese, "[言語:げんご]を[選択:せんたく]してください" },
		{ ELanguage::German,   "Bitte eine Sprache auswählen." },
		{ ELanguage::French,   "Choisissez une langue." },
		{ ELanguage::Spanish,  "Selecciona un idioma." },
		{ ELanguage::Italian,  "Seleziona una lingua." }
	}
},
{
	"Installer_Page_Introduction",
	{
		{ ELanguage::English,  "Welcome to Shadow Recompiled!\n\nYou'll need an Xbox copy of\nShadow the Hedgehog in order to\nproceed with the installation." },
		{ ELanguage::Japanese, "Welcome to Shadow Recompiled!\n\nYou'll need an Xbox copy of\nShadow the Hedgehog in order to\nproceed with the installation." },
		{ ELanguage::German,   "Welcome to Shadow Recompiled!\n\nYou'll need an Xbox copy of\nShadow the Hedgehog in order to\nproceed with the installation." },
		{ ELanguage::French,   "Welcome to Shadow Recompiled!\n\nYou'll need an Xbox copy of\nShadow the Hedgehog in order to\nproceed with the installation." },
		{ ELanguage::Spanish,  "Welcome to Shadow Recompiled!\n\nYou'll need an Xbox copy of\nShadow the Hedgehog in order to\nproceed with the installation." },
		{ ELanguage::Italian,  "Welcome to Shadow Recompiled!\n\nYou'll need an Xbox copy of\nShadow the Hedgehog in order to\nproceed with the installation." }
	}
},
{
	"Installer_Page_SelectGame",
	{
		{ ELanguage::English,  "Add the source for the game." },
		{ ELanguage::Japanese, "Add the source for the game." },
		{ ELanguage::German,   "Add the source for the game." },
		{ ELanguage::French,   "Add the source for the game." },
		{ ELanguage::Spanish,  "Add the source for the game." },
		{ ELanguage::Italian,  "Add the source for the game." }
	}
},
{
	"Installer_Page_NoFileSelected",
	{
		{ ELanguage::English,  "Drag & Drop or Select an ISO or default.xbe" },
		{ ELanguage::Japanese, "Drag & Drop or Select an ISO or default.xbe" },
		{ ELanguage::German,   "Drag & Drop or Select an ISO or default.xbe" },
		{ ELanguage::French,   "Drag & Drop or Select an ISO or default.xbe" },
		{ ELanguage::Spanish,  "Drag & Drop or Select an ISO or default.xbe" },
		{ ELanguage::Italian,  "Drag & Drop or Select an ISO or default.xbe" }
	}
},
{
		// Japanese Notes: This localization should include furigana.
		"Installer_Page_CheckSpace",
		{
			{ ELanguage::English,  "The content will be installed to the program's folder.\n\n" },
			{ ELanguage::Japanese, "コンテンツはプログラムのフォルダに\nインストールされます\n" },
			{ ELanguage::German,   "Der Inhalt wird in dem Ordner des Programms installiert.\n" },
			{ ELanguage::French,   "Le contenu sera installé dans le même dossier que le programme.\n" },
			{ ELanguage::Spanish,  "El contenido será instalado a la carpeta del programa.\n\n" },
			{ ELanguage::Italian,  "Il contenuto verrà installato nella cartella di questo programma.\n\n" }
		}
},
{
		// Japanese Notes: This localization should include furigana.
		"Installer_Step_RequiredSpace",
		{
			{ ELanguage::English,  "Required space: %2.4f GiB" },
			{ ELanguage::Japanese, "[必要:ひつよう]な[容量:ようりょう]: %2.4f GiB" },
			{ ELanguage::German,   "Benötigter Speicherplatz:\n%2.4f GiB" },
			{ ELanguage::French,   "Espace nécessaire :\n%2.4f Gio" },
			{ ELanguage::Spanish,  "Espacio necesario: %2.4f GiB" },
			{ ELanguage::Italian,  "Spazio necessario: %2.4f GiB" }
		}
},
{
		// Japanese Notes: This localization should include furigana.
		"Installer_Step_AvailableSpace",
		{
			{ ELanguage::English,  "Available space: %2.4f GiB" },
			{ ELanguage::Japanese, "[使用:しよう][可能:かのう]な[容量:ようりょう]: %2.4f GiB" },
			{ ELanguage::German,   "Verfügbarer Speicherplatz:\n%2.4f GiB" },
			{ ELanguage::French,   "Espace disponible :\n%2.4f Gio" },
			{ ELanguage::Spanish,  "Espacio disponible: %2.4f GiB" },
			{ ELanguage::Italian,  "Spazio disponibile: %2.4f GiB" }
		}
},
	{
		"Installer_Page_Installing",
		{
			{ ELanguage::English,  "Installing..." },
			{ ELanguage::Japanese, "Installing..." },
			{ ELanguage::German,   "Installing..." },
			{ ELanguage::French,   "Installing..." },
			{ ELanguage::Spanish,  "Installing..." },
			{ ELanguage::Italian,  "Installing..." }
		}
	},
{
	"Installer_Page_Compiling",
	{
		{ ELanguage::English,  "Compiling shaders..." },
		{ ELanguage::Japanese, "Compiling shaders..." },
		{ ELanguage::German,   "Compiling shaders..." },
		{ ELanguage::French,   "Compiling shaders..." },
		{ ELanguage::Spanish,  "Compiling shaders..." },
		{ ELanguage::Italian,  "Compiling shaders..." }
	}
},
{
	"Installer_Page_InstallSucceeded",
	{
		{ ELanguage::English,  "Installation complete!\nThis project is brought to you by:" },
		{ ELanguage::Japanese, "インストール[完了:かんりょう]！\nプロジェクト[制作:せいさく]：" },
		{ ELanguage::German,   "Installation abgeschlossen!\nDieses Projekt wird präsentiert von:" },
		{ ELanguage::French,   "Installation terminée !\nCe projet vous est présenté\npar :" },
		{ ELanguage::Spanish,  "¡Instalación completada!\nEste proyecto ha sido posible gracias a:" },
		{ ELanguage::Italian,  "Installazione completata!\nQuesto progetto è stato creato da:" }
	}
},
{
	"Installer_Page_InstallFailed",
	{
		{ ELanguage::English,  "Installation failed.\n\nAn unknown error occurred." },
		{ ELanguage::Japanese, "Installation failed.\n\nAn unknown error occurred." },
		{ ELanguage::German,   "Installation fehlgeschlagen.\n\nEin unbekannter Fehler ist aufgetreten." },
		{ ELanguage::French,   "L'installation a échoué.\n\nUne erreur inconnue est survenue." },
		{ ELanguage::Spanish,  "La instalación falló.\n\nOcurrió un error desconocido." },
		{ ELanguage::Italian,  "Installazione fallita.\n\nSi è verificato un errore sconosciuto." }
	}
},
{
	"Installer_Step_Game",
	{
		{ ELanguage::English,  "Game" },
		{ ELanguage::Japanese, "ゲーム" },
		{ ELanguage::German,   "Spiel" },
		{ ELanguage::French,   "Jeu" },
		{ ELanguage::Spanish,  "Juego" },
		{ ELanguage::Italian,  "Gioco" }
	}
},
{
	"Installer_Button_Next",
	{
		{ ELanguage::English,  "Next" },
		{ ELanguage::Japanese, "次へ" },
		{ ELanguage::German,   "Weiter" },
		{ ELanguage::French,   "Suivant" },
		{ ELanguage::Spanish,  "Siguiente" },
		{ ELanguage::Italian,  "Continua" }
	}
},
{
	"Installer_Button_Retry",
	{
		{ ELanguage::English,  "Retry" },
		{ ELanguage::Japanese, "リトライ" },
		{ ELanguage::German,   "Erneut versuchen" },
		{ ELanguage::French,   "Réessayer" },
		{ ELanguage::Spanish,  "Reintentar" },
		{ ELanguage::Italian,  "Riprova" }
	}
},
{
	"Installer_Button_Back",
	{
		{ ELanguage::English,  "Back" },
		{ ELanguage::Japanese, "もどる" },
		{ ELanguage::German,   "Zurück" },
		{ ELanguage::French,   "Retour" },
		{ ELanguage::Spanish,  "Atrás" },
		{ ELanguage::Italian,  "Indietro" }
	}
},
{
	"Installer_Button_Quit",
	{
		{ ELanguage::English,  "Quit" },
		{ ELanguage::Japanese, "終了" },
		{ ELanguage::German,   "Verlassen" },
		{ ELanguage::French,   "Quitter" },
		{ ELanguage::Spanish,  "Salir" },
		{ ELanguage::Italian,  "Esci" }
	}
},
{
	"Installer_Button_PlayGame",
	{
		{ ELanguage::English,  "Play Game" },
		{ ELanguage::Japanese, "ゲームを開始" },
		{ ELanguage::German,   "Spiel starten" },
		{ ELanguage::French,   "Jouer" },
		{ ELanguage::Spanish,  "Jugar" },
		{ ELanguage::Italian,  "Giocare" }
	}
},
{
	// Notes: message appears for any install failure
	"Installer_Message_InvalidFileSelected",
	{
		{ ELanguage::English,  "The selected source is not a valid Shadow the Hedgehog Xbox ISO\nor extracted game folder. Please provide a valid one." },
		{ ELanguage::Japanese, "The selected source is not a valid Shadow the Hedgehog Xbox ISO\nor extracted game folder. Please provide a valid one." },
		{ ELanguage::German,   "The selected source is not a valid Shadow the Hedgehog Xbox ISO\nor extracted game folder. Please provide a valid one." },
		{ ELanguage::French,   "The selected source is not a valid Shadow the Hedgehog Xbox ISO\nor extracted game folder. Please provide a valid one." },
		{ ELanguage::Spanish,  "The selected source is not a valid Shadow the Hedgehog Xbox ISO\nor extracted game folder. Please provide a valid one." },
		{ ELanguage::Italian,  "The selected source is not a valid Shadow the Hedgehog Xbox ISO\nor extracted game folder. Please provide a valid one." }
	}
},
{
	// Notes: message appears when pressing B at the title screen.
	"Title_Message_Quit",
	{
		{ ELanguage::English,  "Are you sure you want to quit the game?" },
		{ ELanguage::Japanese, "[終了:しゅうりょう]してもよろしいですか？" },
		{ ELanguage::German,   "Spiel verlassen?" },
		{ ELanguage::French,   "Êtes-vous sûr de vouloir quitter le jeu ?" },
		{ ELanguage::Spanish,  "¿Estás seguro de que quieres\nsalir?" },
		{ ELanguage::Italian,  "Sicuro di voler uscire ?" }
	}
},
{
	// Notes: message appears when creating a save file at the title screen.
	// Note, the game exclusively use "Game Data" when referring to save file, please re use this for consistency.
	"Title_Message_CreateSave",
	{
		{ ELanguage::English,  "There is no Game Data on your PC.\nSHADOW THE HEDGEHOG™\nsupports an autosave feature.\nDo you want to create Game Data?" },
		{ ELanguage::Japanese, "There is no Game Data on your PC.\nSHADOW THE HEDGEHOG™\nsupports an autosave feature.\nDo you want to create Game Data?" },
		{ ELanguage::German,   "There is no Game Data on your PC.\nSHADOW THE HEDGEHOG™\nsupports an autosave feature.\nDo you want to create Game Data?" },
		{ ELanguage::French,   "Pas de données de jeu sur ce PC.\nSHADOW THE HEDGEHOG™\ndispose d'un système de sauvegarde automatique.\nCréer des données de jeu sur\nce PC?" },
		{ ELanguage::Spanish,  "There is no Game Data on your PC.\nSHADOW THE HEDGEHOG™\nsupports an autosave feature.\nDo you want to create Game Data?" },
		{ ELanguage::Italian,  "There is no Game Data on your PC.\nSHADOW THE HEDGEHOG™\nsupports an autosave feature.\nDo you want to create Game Data?" }
	}
},
{
	// Notes: message appears when creating a save file at the title screen.
	// Note, the game exclusively use "Game Data" when referring to save file, please re use this for consistency.
	"Title_Message_CreateSaveFailed",
	{
		{ ELanguage::English,  "Start the game without creating\nGame Data.\nProgress of the game will not be saved." },
		{ ELanguage::Japanese, "Start the game without creating\nGame Data.\nProgress of the game will not be saved." },
		{ ELanguage::German,   "Start the game without creating\nGame Data.\nProgress of the game will not be saved." },
		{ ELanguage::French,   "Démarrer le jeu sans créer de données du jeu.\nLa progression du jeu ne sera\npas sauvegardée." },
		{ ELanguage::Spanish,  "Start the game without creating\nGame Data.\nProgress of the game will not be saved." },
		{ ELanguage::Italian,  "Start the game without creating\nGame Data.\nProgress of the game will not be saved." }
	}
},
// Notes: on load if the save file is corrupted
{
	"Title_Message_SaveDamaged",
	{
		{ ELanguage::English,  "The Game Data is damaged\nand cannot be used.\nPress A button to continue." },
		{ ELanguage::Japanese, "The Game Data is damaged\nand cannot be used.\nPress A button to continue." },
		{ ELanguage::German,   "The Game Data is damaged\nand cannot be used.\nPress A button to continue." },
		{ ELanguage::French,   "Les données de jeu sont endommagées.\nAppuyer sur A pour continuer." },
		{ ELanguage::Spanish,  "The Game Data is damaged\nand cannot be used.\nPress A button to continue." },
		{ ELanguage::Italian,  "The Game Data is damaged\nand cannot be used.\nPress A button to continue." },
	}
},
// Notes: same as above but without "Press A"
{
	"Title_Message_SaveDamagedNoA",
	{
		{ ELanguage::English,  "The Game Data is damaged\nand cannot be used." },
		{ ELanguage::Japanese, "The Game Data is damaged\nand cannot be used." },
		{ ELanguage::German,   "Spieldaten sind beschädigt und\nkönnen nicht verwendet werden." },
		{ ELanguage::French,   "Les données de jeu sont endommagées." },
		{ ELanguage::Spanish,  "Los datos de juego están dañados." },
		{ ELanguage::Italian,  "I dati di gioco sono danneggiati." },
	}
},
// Notes: If the game fails to save, rw lock or otherwise
{
	"Title_Message_SaveFailed",
	{
		{ ELanguage::English,  "Save failed." },
		{ ELanguage::Japanese, "Save failed." },
		{ ELanguage::German,   "Speichern gescheitert." },
		{ ELanguage::French,   "Echec de la sauvegarde." },
		{ ELanguage::Spanish,  "Error al guardar." },
		{ ELanguage::Italian,  "Salvataggio fallito." },
	}
},
{
	"Title_Message_OverwriteSave",
	{
		{ ELanguage::English,  "Do you wish to overwrite the Game Data?\nCaution! This will overwrite any\nprevious Game Data present." },
		{ ELanguage::Japanese, "Do you wish to overwrite the Game Data?\nCaution! This will overwrite any\nprevious Game Data present." },
		{ ELanguage::German,   "Do you wish to overwrite the Game Data?\nCaution! This will overwrite any\nprevious Game Data present." },
		{ ELanguage::French,   "Voulez-vous écraser les données de jeu ?\nAttention, cela écrasera toutes les données de jeu précédentes." },
		{ ELanguage::Spanish,  "Do you wish to overwrite the Game Data?\nCaution! This will overwrite any\nprevious Game Data present." },
		{ ELanguage::Italian,  "Do you wish to overwrite the Game Data?\nCaution! This will overwrite any\nprevious Game Data present." },
	}
},
{
	"Legacy_Options_Message_OverwriteSave",
	{
		{ ELanguage::English,  "Do you want to load Game Data?\nCaution! This will overwrite any\nprevious Game Data present." },
		{ ELanguage::Japanese, "Do you want to load Game Data?\nCaution! This will overwrite any\nprevious Game Data present." },
		{ ELanguage::German,   "Do you want to load Game Data?\nCaution! This will overwrite any\nprevious Game Data present." },
		{ ELanguage::French,   "Voulez-vous charger les données de jeu ?\nAttention, cela écrasera toutes les données de jeu précédentes." },
		{ ELanguage::Spanish,  "Do you want to load Game Data?\nCaution! This will overwrite any\nprevious Game Data present." },
		{ ELanguage::Italian,  "Do you want to load Game Data?\nCaution! This will overwrite any\nprevious Game Data present." },
	}
},
{
	"Legacy_Options_Message_LoadSaveOK",
	{
		{ ELanguage::English,  "Load successful." },
		{ ELanguage::Japanese, "Load successful." },
		{ ELanguage::German,   "Load successful." },
		{ ELanguage::French,   "Chargement réussi." },
		{ ELanguage::Spanish,  "Load successful." },
		{ ELanguage::Italian,  "Load successful." },
	}
},
{
	// Notes: message appears when creating a save file at the title screen.
	// Note, the game exclusively use "Game Data" when referring to save file, please re use this for consistency.
	"Title_Message_LoadSave",
	{
		{ ELanguage::English,  "Do you want to load Game Data?" },
		{ ELanguage::Japanese, "Do you want to load Game Data?" },
		{ ELanguage::German,   "Do you want to load Game Data?" },
		{ ELanguage::French,   "Souhaitez-vous charger les\n données de jeu?" },
		{ ELanguage::Spanish,  "Do you want to load Game Data?" },
		{ ELanguage::Italian,  "Do you want to load Game Data?" },
	}
},
{
	// Notes: message appears when creating a save file at the title screen.
	// Note, the game exclusively use "Game Data" when referring to save file, please re use this for consistency.
	"Title_Message_ContinueWithoutSave",
	{
		// there is a space added before the question mark to match the original game text, even in english.
		{ ELanguage::English,  "Continue without saving ?" },
		{ ELanguage::Japanese, "Continue without saving ?" },
		{ ELanguage::German,   "Continue without saving ?" },
		{ ELanguage::French,   "Continuez sans sauvegarder ?" },
		{ ELanguage::Spanish,  "Continue without saving ?" },
		{ ELanguage::Italian,  "Continue without saving ?" },
	}
},
{
	// Notes: message appears when choosing restart in a stage.
	"Pause_Message_Restart",
	{
		{ ELanguage::English,  "Are you sure you want to restart the\nstage? Your progress in the current\nstage will be lost if you restart." },
		{ ELanguage::Japanese, "Are you sure you want to restart the\nstage? Your progress in the current\nstage will be lost if you restart." },
		{ ELanguage::German,   "Sind Sie sicher das Sie die Ebene\nneu beginnen wollen? Der Spielfortschritt\ndieser Ebene wird dabei gelöscht." },
		{ ELanguage::French,   "Recommencer l'étape ?\nVotre progression sur cette étape\nsera perdue si vous recommencez." },
		{ ELanguage::Spanish,  "¿Quieres volver a empezar esta etapa?\nTodo el progreso en esta etapa se\nperderá si vuelves a empezar." },
		{ ELanguage::Italian,  "Sicuro di voler ricominciare il quadro?\nI progressi di questo quadro andranno\npersi se ricominci." },
	}
},
{
	// Notes: message appears when choosing quit in a stage.
	"Pause_Message_Quit",
	{
		{ ELanguage::English,  "Are you sure you want to quit the game?\nYour progress in the current stage\nwill be lost if you quit." },
		{ ELanguage::Japanese, "Are you sure you want to quit the game?\nYour progress in the current stage\nwill be lost if you quit." },
		{ ELanguage::German,   "Sind Sie sicher das Sie das Spiel\nabbrechen wollen?\nSie verlieren dabei Ihren Fortschritt\nin dieser Ebene." },
		{ ELanguage::French,   "Quitter la partie ?\nVotre progression sur cette étape\nsera perdue si vous quittez." },
		{ ELanguage::Spanish,  "¿Quieres abandonar la partida?\nTodo el progreso en esta etapa se\nperderá si abandonas." },
		{ ELanguage::Italian,  "Sicuro di voler uscire dal gioco?\nI progressi di questo quadro andranno\npersi se esci dal gioco." },
	}
},
{
	"Unlock_Message_LastStory",
	{
		{ ELanguage::English,  "LAST STORY unlocked!" },
		{ ELanguage::Japanese, "LAST STORY unlocked!" },
		{ ELanguage::German,   "LETZTE STORY freigeschaltet!" },
		{ ELanguage::French,   "DERNIERE HISTOIRE déverrouillée !" },
		{ ELanguage::Spanish,  "¡HISTORIA FINAL desbloqueada!" },
		{ ELanguage::Italian,  "STORIA FINALE sbloccata!" },
	}
},
{
	// Notes: the '.' in "EXPERT MODE." is an English only original typo
	"Unlock_Message_ExpertMode",
	{
		{ ELanguage::English,  "EXPERT MODE unlocked!" },
		{ ELanguage::Japanese, "EXPERT MODE unlocked!" },
		{ ELanguage::German,   "EXPERTEN-MODUS freigeschaltet!" },
		{ ELanguage::French,   "MODE EXPERT déverrouillé !" },
		{ ELanguage::Spanish,  "¡MODO EXPERTO desbloqueado!" },
		{ ELanguage::Italian,  "MODALITÀ ESPERTO sbloccata!" },
	}
},
{
	"Unlock_Message_SamuraiBladeLv1",
	{
		{ ELanguage::English,  "You unlocked \"Samurai Blade\"." },
		{ ELanguage::Japanese, "You unlocked \"Samurai Blade\"." },
		{ ELanguage::German,   "Du hast \"Samurai Blade\" freigeschaltet." },
		{ ELanguage::French,   "Vous avez déverrouillé \"Samurai Blade\"." },
		{ ELanguage::Spanish,  "Has desbloqueado \"Samurai Blade\"." },
		{ ELanguage::Italian,  "Hai sbloccato \"Samurai Blade\"." },
	}
},
{
	"Unlock_Message_SamuraiBladeLv2",
	{
		{ ELanguage::English,  "\"Samurai Blade\" is now Level 2." },
		{ ELanguage::Japanese, "\"Samurai Blade\" is now Level 2." },
		{ ELanguage::German,   "\"Samurai Blade\" hat Level 2 erreicht." },
		{ ELanguage::French,   "Vous avez atteint le niveau 2 de \"Samurai Blade\"." },
		{ ELanguage::Spanish,  "\"Samurai Blade\" Nivel 2." },
		{ ELanguage::Italian,  "\"Samurai Blade\" ha raggiunto il livello 2." },
	}
},
{
	"Unlock_Message_SatelliteLaserLv1",
	{
		{ ELanguage::English,  "You unlocked \"Satellite Laser\"." },
		{ ELanguage::Japanese, "You unlocked \"Satellite Laser\"." },
		{ ELanguage::German,   "Du hast \"Satellite Laser\" freigeschaltet." },
		{ ELanguage::French,   "Vous avez déverrouillé \"Satellite Laser\"." },
		{ ELanguage::Spanish,  "Has desbloqueado \"Satellite Laser\"." },
		{ ELanguage::Italian,  "Hai sbloccato \"Satellite Laser\"." },
	}
},
{
	"Unlock_Message_SatelliteLaserLv2",
	{
		{ ELanguage::English,  "\"Satellite Laser\" is now Level 2." },
		{ ELanguage::Japanese, "\"Satellite Laser\" is now Level 2." },
		{ ELanguage::German,   "\"Satellite Laser\" hat Level 2 erreicht." },
		{ ELanguage::French,   "Vous avez atteint le niveau 2 de \"Satellite Laser\"." },
		{ ELanguage::Spanish,  "\"Satellite Laser\" Nivel 2." },
		{ ELanguage::Italian,  "\"Satellite Laser\" ha raggiunto il livello 2." },
	}
},
{
	"Unlock_Message_VacuumEggLv1",
	{
		{ ELanguage::English,  "You unlocked \"Vacuum Egg\"." },
		{ ELanguage::Japanese, "You unlocked \"Vacuum Egg\"." },
		{ ELanguage::German,   "Du hast \"Vacuum Egg\" freigeschaltet." },
		{ ELanguage::French,   "Vous avez déverrouillé \"Vacuum Egg\"." },
		{ ELanguage::Spanish,  "Has desbloqueado \"Vacuum Egg\"." },
		{ ELanguage::Italian,  "Hai sbloccato \"Vacuum Egg\"." },
	}
},
{
	"Unlock_Message_VacuumEggLv2",
	{
		{ ELanguage::English,  "\"Vacuum Egg\" is now Level 2." },
		{ ELanguage::Japanese, "\"Vacuum Egg\" is now Level 2." },
		{ ELanguage::German,   "\"Vacuum Egg\" hat Level 2 erreicht." },
		{ ELanguage::French,   "Vous avez atteint le niveau 2 de \"Vacuum Egg\"." },
		{ ELanguage::Spanish,  "\"Vacuum Egg\" Nivel 2." },
		{ ELanguage::Italian,  "\"Vacuum Egg\" ha raggiunto il livello 2." },
	}
},
{
	"Unlock_Message_OmochaoGunLv1",
	{
		{ ELanguage::English,  "You unlocked \"Omochao Gun\"." },
		{ ELanguage::Japanese, "You unlocked \"Omochao Gun\"." },
		{ ELanguage::German,   "Du hast \"Omochao Gun\" freigeschaltet." },
		{ ELanguage::French,   "Vous avez déverrouillé \"Omochao Gun\"." },
		{ ELanguage::Spanish,  "Has desbloqueado \"Omochao Gun\"." },
		{ ELanguage::Italian,  "Hai sbloccato \"Omochao Gun\"." },
	}
},
{
	"Unlock_Message_OmochaoGunLv2",
	{
		{ ELanguage::English,  "\"Omochao Gun\" is now Level 2." },
		{ ELanguage::Japanese, "\"Omochao Gun\" is now Level 2." },
		{ ELanguage::German,   "\"Omochao Gun\" hat Level 2 erreicht." },
		{ ELanguage::French,   "Vous avez atteint le niveau 2 de \"Omochao Gun\"." },
		{ ELanguage::Spanish,  "\"Omochao Gun\" Nivel 2." },
		{ ELanguage::Italian,  "\"Omochao Gun\" ha raggiunto il livello 2." },
	}
},
{
	"Unlock_Message_HealCannonLv1",
	{
		{ ELanguage::English,  "You unlocked \"Heal Cannon\"." },
		{ ELanguage::Japanese, "You unlocked \"Heal Cannon\"." },
		{ ELanguage::German,   "Du hast \"Heal Cannon\" freigeschaltet." },
		{ ELanguage::French,   "Vous avez déverrouillé \"Heal Cannon\"." },
		{ ELanguage::Spanish,  "Has desbloqueado \"Heal Cannon\"." },
		{ ELanguage::Italian,  "Hai sbloccato \"Heal Cannon\"." },
	}
},
{
	"Unlock_Message_HealCannonLv2",
	{
		{ ELanguage::English,  "\"Heal Cannon\" is now Level 2." },
		{ ELanguage::Japanese, "\"Heal Cannon\" is now Level 2." },
		{ ELanguage::German,   "\"Heal Cannon\" hat Level 2 erreicht." },
		{ ELanguage::French,   "Vous avez atteint le niveau 2 de \"Heal Cannon\"." },
		{ ELanguage::Spanish,  "\"Heal Cannon\" Nivel 2." },
		{ ELanguage::Italian,  "\"Heal Cannon\" ha raggiunto il livello 2." },
	}
},
{
	"Unlock_Message_ShadowRifle",
	{
		{ ELanguage::English,  "You unlocked \"Shadow Rifle\"." },
		{ ELanguage::Japanese, "You unlocked \"Shadow Rifle\"." },
		{ ELanguage::German,   "Du hast \"Shadow Rifle\" freigeschaltet." },
		{ ELanguage::French,   "Vous avez déverrouillé \"Shadow Rifle\"." },
		{ ELanguage::Spanish,  "Has desbloqueado \"Shadow Rifle\"." },
		{ ELanguage::Italian,  "Hai sbloccato \"Shadow Rifle\"." },
	}
},
{
	"Title_Message_UpdateAvailable",
	{
		{ ELanguage::English,  "An update is available!\n\nWould you like to visit the\nreleases page to download it?" },
		{ ELanguage::Japanese, "アップデートが[利用:りよう][可能:かのう]です\n\nリリースページにアクセスして\nダウンロードしますか？" },
		{ ELanguage::German,   "Ein Update ist verfügbar!\n\nMöchtest du die Release-Seite\nbesuchen um es herunterzuladen?" },
		{ ELanguage::French,   "Une mise à jour est disponible !\n\nVoulez-vous visiter la page\ndes mises à jour pour la\ntélécharger ?" },
		{ ELanguage::Spanish,  "¡Hay una actualización disponible!\n\n¿Quieres ir a la página\npara descargarla?" },
		{ ELanguage::Italian,  "È disponibile un aggiornamento!\n\nVuoi visitare la pagina releases\nper scaricarlo?" }
	}
},
{
	"Video_BackendError",
	{
		{ ELanguage::English,  "Unable to create a D3D12 (Windows) or Vulkan backend.\n\nPlease make sure that:\n\n- Your system meets the minimum requirements.\n- Your GPU drivers are up to date.\n- Your operating system is on the latest version available." },
		{ ELanguage::Japanese, "D3D12 (Windows)または\nVulkanバックエンドを作成できません\n\n次の点を確認してください：\n\n※システムが最小要件を満たしている\n※GPUドライバーが最新である\n※オペレーティングシステムが最新バージョンである" },
		{ ELanguage::German,   "Es ist nicht möglich, ein D3D12 (Windows) oder Vulkan-Backend zu erstellen.\n\nBitte stelle sicher, dass:\n\n- Dein System die Mindestanforderungen erfüllt.\n- Deine GPU-Treiber auf dem neuesten Stand sind.\n- Dein Betriebssystem auf der neuesten verfügbaren Version ist." },
		{ ELanguage::French,   "Impossible de créer un backend D3D12 (Windows) ou Vulkan.\n\nVeuillez vous assurer que :\n\n- Votre système répond aux critères minimums requis.\n- Les pilotes de votre processeur graphique sont à jour.\n- Votre système d'exploitation est à jour." },
		{ ELanguage::Spanish,  "No se puede crear un entorno de D3D12 (Windows) o de Vulkan.\n\nPor favor, asegúrate de que:\n\n- Tu equipo cumple con los requisitos mínimos.\n- Los drivers de tu tarjeta gráfica están actualizados.\n- Tu sistema operativo está actualizado a la última versión.\n" },
		{ ELanguage::Italian,  "Impossibile creare un backend D3D12 (Windows) o Vulkan.\n\nAssicurati che:\n\n- Il tuo sistema soddisfi i requisiti minimi.\n- I driver della scheda grafica siano aggiornati.\n- Il tuo sistema operativo sia aggiornato." }
	}
},
{
	"System_MemoryAllocationFailed",
	{
		{ ELanguage::English,  "Failed to allocate game memory.\n\nPlease make sure that:\n\n- You meet the minimum system requirements (8 GB).\n- Your page file is configured with at least 4-8 GB of virtual memory." },
		{ ELanguage::Japanese, "ゲームメモリの割り当てに失敗しました\n\n次の点を確認してください：\n\n※最小システム要件（8 GB）を満たしていること。\n※ページファイルに少なくとも4～8 GBの仮想メモリが設定されていること" },
		{ ELanguage::German,   "Fehler beim Zuweisen des Spielspeichers.\n\nBitte stelle sicher, dass:\n\n- Die Mindestanforderungen für das System erfüllt sind (8 GB).\n- Die Auslagerungsdatei mit mindestens 4-8 GB virtuellem Speicher konfiguriert ist." },
		{ ELanguage::French,   "Échec d'allocation de la mémoire du jeu.\n\nVeuillez vous assurer que :\n\n- Vous disposez de la configuration minimale requise (8 Go).\n- Votre fichier d’échange est configuré avec au moins 4 à 8 Go de mémoire virtuelle." },
		{ ELanguage::Spanish,  "Fallo al asignar memoria del juego.\n\nPor favor, asegúrate de que:\n\n- Cumples los requisitos mínimos del sistema (8 GB).\n- Tu archivo de páginación está configurado con al menos 4 u 8 GB de memoria virtual." },
		{ ELanguage::Italian,  "Impossibile allocare la memoria per il gioco.\n\nAssicurati che:\n\n- Soddisfi i requisiti minimi di sistema (8 GB).\n- Il tuo file di paging sia configurato con almeno 4 o 8 GB di memoria virtuale." }
	}
},
{
	"IntegrityCheck_Success",
	{
		{ ELanguage::English,  "Installation check has finished.\n\nAll files seem to be correct.\n\nThe game will now close. Remove the launch argument to play the game." },
		{ ELanguage::Japanese, "インストールチェックが終了しました\n\nすべてのファイルは正しいようです\n\nゲームは終了します、ゲームをプレイするには起動引数を削除してください" },
		{ ELanguage::German,   "Die Installation wurde überprüft.\n\nAlle Dateien scheinen korrekt zu sein.\n\nDas Spiel wird nun geschlossen. Entferne die Startoption, um das Spiel zu spielen." },
		{ ELanguage::French,   "La vérification de l'installation est terminée.\n\nTous les fichiers semblent corrects.\n\nL'application va maintenant se fermer. Retirez l'argument de lancement pour pouvoir lancer le jeu." },
		{ ELanguage::Spanish,  "La verificación de la instalación ha terminado.\n\nTodos los archivos parecen correctos.\n\nEl juego se cerrará ahora. Elimina el argumento de lanzamiento para jugar al juego." },
		{ ELanguage::Italian,  "La verifica dei file d'installazione è terminata.\n\nTutti i file sembrano corretti.\n\nIl gioco si chiuderà. Rimuovi l'argomento di avvio per poter giocare." }
	}
},
{
	"IntegrityCheck_Failed",
	{
		{ ELanguage::English,  "Installation check has failed.\n\nError: %s\n\nThe game will now close. Try reinstalling the game by using the --install launch argument." },
		{ ELanguage::Japanese, "インストールチェックに失敗しました\n\nエラー：%s\n\nゲームは終了します、--install 起動引数を使用してゲームを再インストールしてください" },
		{ ELanguage::German,   "Die Installationsprüfung ist fehlgeschlagen.\n\nFehler: %s\n\nDas Spiel wird nun geschlossen. Versuche das Spiel durch Verwendung der Startoption --install neu zu installieren." },
		{ ELanguage::French,   "La vérification de l'installation a échoué.\n\nErreur : %s\n\nL'application va maintenant se fermer. Essayez de réinstaller le jeu en utilisant l'argument de lancement --install." },
		{ ELanguage::Spanish,  "La verificación de la instalación ha fallado.\n\nError: %s\n\nEl juego se cerrará ahora. Intenta reinstalar el juego utilizando el argumento de lanzamiento --install." },
		{ ELanguage::Italian,  "La verifica dei file d'installazione non è andata a buon fine.\n\nErrore: %s\n\nIl gioco si chiuderà. Prova a reinstallare il gioco utilizzando l'argomento di avvio --install." }
	}
},
{
	"Common_On",
	{
		{ ELanguage::English,  "ON" },
		{ ELanguage::Japanese, "オン" },
		{ ELanguage::German,   "EIN" },
		{ ELanguage::French,   "OUI" },
		{ ELanguage::Spanish,  "SÍ" },
		{ ELanguage::Italian,  "SÌ" }
	}
},
{
	"Common_Off",
	{
		{ ELanguage::English,  "OFF" },
		{ ELanguage::Japanese, "オフ" },
		{ ELanguage::German,   "AUS" },
		{ ELanguage::French,   "NON" },
		{ ELanguage::Spanish,  "NO" },
		{ ELanguage::Italian,  "NO" }
	}
},
{
	"Common_Yes",
	{
		{ ELanguage::English,  "Yes" },
		{ ELanguage::Japanese, "はい" },
		{ ELanguage::German,   "Ja" },
		{ ELanguage::French,   "Oui" },
		{ ELanguage::Spanish,  "Sí" },
		{ ELanguage::Italian,  "Sì" }
	}
},
{
	"Common_No",
	{
		{ ELanguage::English,  "No" },
		{ ELanguage::Japanese, "いいえ" },
		{ ELanguage::German,   "Nein" },
		{ ELanguage::French,   "Non" },
		{ ELanguage::Spanish,  "No" },
		{ ELanguage::Italian,  "No" }
	}
},
{
	"Common_Ok",
	{
		{ ELanguage::English,  "OK" },
		{ ELanguage::Japanese, "OK" },
		{ ELanguage::German,   "OK" },
		{ ELanguage::French,   "OK" },
		{ ELanguage::Spanish,  "OK" },
		{ ELanguage::Italian,  "OK" }
	}
},
{
	"Common_Select",
	{
		{ ELanguage::English,  "select" },
		{ ELanguage::Japanese, "決定" },
		{ ELanguage::German,   "auswählen" },
		{ ELanguage::French,   "confirmer" },
		{ ELanguage::Spanish,  "elegir" },
		{ ELanguage::Italian,  "seleziona" }
	}
},
{
	"Common_Back",
	{
		{ ELanguage::English,  "back" },
		{ ELanguage::Japanese, "もどる" },
		{ ELanguage::German,   "zurück" },
		{ ELanguage::French,   "retour" },
		{ ELanguage::Spanish,  "atrás" },
		{ ELanguage::Italian,  "indietro" }
	}
},
{
	"Common_Quit",
	{
		{ ELanguage::English,  "quit" },
		{ ELanguage::Japanese, "終了" },
		{ ELanguage::German,   "verlassen" },
		{ ELanguage::French,   "quitter" },
		{ ELanguage::Spanish,  "salir" },
		{ ELanguage::Italian,  "esci" }
	}
},
{
	"Common_Cancel",
	{
		{ ELanguage::English,  "cancel" },
		{ ELanguage::Japanese, "キャンセル" },
		{ ELanguage::German,   "abbrechen" },
		{ ELanguage::French,   "annuler" },
		{ ELanguage::Spanish,  "cancelar" },
		{ ELanguage::Italian,  "annulla" }
	}
},
{
	"Common_Reset",
	{
		{ ELanguage::English,  "reset" },
		{ ELanguage::Japanese, "リセット" },
		{ ELanguage::German,   "zurücksetzen" },
		{ ELanguage::French,   "par défaut" },
		{ ELanguage::Spanish,  "reiniciar" },
		{ ELanguage::Italian,  "ripristina" }
	}
},
{
	"Common_Exit",
	{
		{ ELanguage::English,  "exit" },
		{ ELanguage::Japanese, "終了" },
		{ ELanguage::German,   "verlassen" },
		{ ELanguage::French,   "quitter" },
		{ ELanguage::Spanish,  "salir" },
		{ ELanguage::Italian,  "esci" }
	}
},
{
	"Common_Switch",
	{
		{ ELanguage::English,  "switch" },
		{ ELanguage::Japanese, "きりかえ" },
		{ ELanguage::German,   "wechseln" },
		{ ELanguage::French,   "changer" },
		{ ELanguage::Spanish,  "cambiar" },
		{ ELanguage::Italian,  "cambia" }
	}
},
{
	"Common_File",
	{
		{ ELanguage::English,  "File" },
		{ ELanguage::Japanese, "ファイル" },
		{ ELanguage::German,   "Datei" },
		{ ELanguage::French,   "Fichier" },
		{ ELanguage::Spanish,  "Archivo" },
		{ ELanguage::Italian,  "File" }
	}
},
{
	"Common_Install",
	{
		{ ELanguage::English,  "Install" },
		{ ELanguage::Japanese, "インストール" },
		{ ELanguage::German,   "Installieren" },
		{ ELanguage::French,   "Installer" },
		{ ELanguage::Spanish,  "Instalar" },
		{ ELanguage::Italian,  "Installa" }
	}
}
};

std::string& Localise(const std::string_view& key)
{
	auto localeFindResult = g_locale.find(key);
	if (localeFindResult != g_locale.end())
	{
		auto languageFindResult = localeFindResult->second.find(Config::Language);
		if (languageFindResult == localeFindResult->second.end())
			languageFindResult = localeFindResult->second.find(ELanguage::English);

		if (languageFindResult != localeFindResult->second.end())
			return languageFindResult->second;
	}

	return g_localeMissing;
}
