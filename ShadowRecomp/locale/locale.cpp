#include <user/config.h>
#include <locale/locale.h>

/*
	!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! LOCALISATION NOTES !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	- Ensure brand names are always presented on the same line.

	  Correct:
	  This is a string that contains a brand name like
	  Xbox, which is one of the few consoles to have a port of
	  Shadow the Hedgehog.

	  Incorrect:
	  This is a string that contains a brand name like Xbox,
	  which is one of the few consoles to have a port of Shadow
	  the Hedgehog.

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

std::unordered_map<std::string_view, std::unordered_map<ELanguage, std::string>> g_locale =
{
	{
		"Options_Header_Name",
		{
			{ ELanguage::English,  "OPTIONS" },
			{ ELanguage::Japanese, "OPTION" },
			{ ELanguage::German,   "OPTIONEN" },
			{ ELanguage::French,   "OPTIONS" },
			{ ELanguage::Spanish,  "OPCIONES" },
			{ ELanguage::Italian,  "OPZIONI" }
		}
	},
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
		"Options_Category_Misc",
		{
			{ ELanguage::English,  "MISC" },
			{ ELanguage::Japanese, "MISC" },
			{ ELanguage::German,   "SONSTIGES" },
			{ ELanguage::French,   "DIVERS" },
			{ ELanguage::Spanish,  "VARIOS" },
			{ ELanguage::Italian,  "VARI" }
		}
	},
	{
		"Options_Desc_Category_Misc",
		{
			{ ELanguage::English,  "Adjust miscellaneous settings." },
			{ ELanguage::Japanese, "その他の設定を変更します" },
			{ ELanguage::German,   "Verändere sonstige Einstellungen." },
			{ ELanguage::French,   "Modifier les paramètres divers." },
			{ ELanguage::Spanish,  "Ajustar la configuración diversa." },
			{ ELanguage::Italian,  "Modifica le impostazioni varie." }
		}
	},
	{
		"Options_Category_Debug",
		{
			{ ELanguage::English,  "Debug settings" },
			{ ELanguage::Japanese, "デバッグ設定" },
			{ ELanguage::German,   "Debug-Einstellungen" },
			{ ELanguage::French,   "Paramètres de débogage" },
			{ ELanguage::Spanish,  "Configuración de depuración" },
			{ ELanguage::Italian,  "Impostazioni debug" }
		}
			},
	{
		"Options_Desc_Category_Debug",
		{
			{ ELanguage::English,  "Adjust debug settings." },
			{ ELanguage::Japanese, "デバッグの設定を変更します" },
			{ ELanguage::German,   "Verändere Debug-Einstellungen." },
			{ ELanguage::French,   "Modifier les paramètres de débogage." },
			{ ELanguage::Spanish,  "Ajustar la configuración de depuración." },
			{ ELanguage::Italian,  "Modifica le impostazioni debug." }
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
		"Options_Name_LegacyMenu",
		{
			{ ELanguage::English,  "Legacy Options" },
			{ ELanguage::Japanese, "レガシーオプション" },
			{ ELanguage::German,   "Legacy-Optionen" },
			{ ELanguage::French,   "Options Héritées" },
			{ ELanguage::Spanish,  "Opciones Heredadas" },
			{ ELanguage::Italian,  "Opzioni	Legacy" }
		}
	},
	{
		// Japanese Notes: This localization should include furigana.
		"Options_Desc_LegacyMenu",
		{
			{ ELanguage::English,  "Go to Legacy Options" },
			{ ELanguage::Japanese, "レガシーオプションに移動" },
			{ ELanguage::German,   "Legacy-Optionen aufrufen" },
			{ ELanguage::French,   "Aller aux options Héritées" },
			{ ELanguage::Spanish,  "Ir a las opciones Heredadas" },
			{ ELanguage::Italian,  "Vai alle opzioni Legacy" }
		}
	},

	{
		"Options_Name_WindowSize",
		{
			{ ELanguage::English,  "Resolution" },
			{ ELanguage::Japanese, "解像度" },
			{ ELanguage::German,   "Auflösung" },
			{ ELanguage::French,   "Résolution" },
			{ ELanguage::Spanish,  "Resolución" },
			{ ELanguage::Italian,  "Risoluzione" }
		}
	},
	{
		// Japanese Notes: This localization should include furigana.
		"Options_Desc_WindowSize",
		{
			{ ELanguage::English,  "Adjust the resolution of the game in windowed mode." },
			{ ELanguage::Japanese, "ゲームの\u200B[解像度:かいぞうど]を\u200B[設定:せってい]できます" },
			{ ELanguage::German,   "Passe die Auflösung des Spiels im Fenstermodus an." },
			{ ELanguage::French,   "Définir la résolution de jeu en mode fenêtré." },
			{ ELanguage::Spanish,  "Ajusta la resolución del juego en modo ventana." },
			{ ELanguage::Italian,  "Regola la risoluzione del gioco in modalità finestra." }
		}
	},
	{
		// Notes: description for options that cannot be accessed anywhere but the title screen or world map (e.g. Language).
		// Japanese Notes: This localization should include furigana.
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
		// Notes: currently the description for Window Size when in fullscreen.
		// Japanese Notes: This localization should include furigana.
		"Options_Desc_NotAvailableFullscreen",
		{
			{ ELanguage::English,  "This option is not available in fullscreen mode." },
			{ ELanguage::Japanese, "この\u200Bオプションは\u200Bフルスクリーンモードでは\u200B[変更:へんこう]\u200Bできません" },
			{ ELanguage::German,   "Diese Option ist im Vollbildmodus nicht verfügbar." },
			{ ELanguage::French,   "Cette option n'est pas disponible en mode plein écran." },
			{ ELanguage::Spanish,  "Esta opción no está disponible en modo pantalla completa." },
			{ ELanguage::Italian,  "Questa opzione non è disponibile in modalità schermo pieno." }
		}
	},
	{
		// Notes: currently the description for Monitor when in fullscreen.
		// Japanese Notes: This localization should include furigana.
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
		// Japanese Notes: This localization should include furigana.
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
		// Notes: description for Transparency Anti-Aliasing when MSAA is disabled.
		// Japanese Notes: This localization should include furigana.
		"Options_Desc_NotAvailableMSAA",
		{
			{ ELanguage::English,  "This option is not available without MSAA." },
			{ ELanguage::Japanese, "この\u200Bオプションは\u200BMSAAなしで\u200B[変更:へんこう]\u200Bできません" },
			{ ELanguage::German,   "Diese Option ist ohne MSAA nicht verfügbar." },
			{ ELanguage::French,   "Cette option n'est pas disponible sans MSAA." },
			{ ELanguage::Spanish,  "Esta opción no está disponible sin MSAA." },
			{ ELanguage::Italian,  "Questa opzione non è disponibile senza MSAA." }
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
		// Notes: message appears when changing the Language option and backing out of the options menu.
		// Japanese Notes: This localization should include furigana.
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
		// Japanese Notes: This localization should include furigana.
		"Pause_Menu_Continue",
		{
			{ ELanguage::English,  "resume" },
			{ ELanguage::Japanese, "" },
			{ ELanguage::German,   "weiter" },
			{ ELanguage::French,   "reprendre" },
			{ ELanguage::Spanish,  "continuar" },
			{ ELanguage::Italian,  "continua" }
		}
	},
	{
		// Notes: text in pause menu during gameplay
		// Japanese Notes: This localization should include furigana.
		"Pause_Menu_Restart",
		{
			{ ELanguage::English,  "restart" },
			{ ELanguage::Japanese, "" },
			{ ELanguage::German,   "neustart" },
			{ ELanguage::French,   "redémarrer" },
			{ ELanguage::Spanish,  "volver a empezar" },
			{ ELanguage::Italian,  "ricomincia" }
		}
	},
	{
		// Notes: text in pause menu during gameplay
		// Japanese Notes: This localization should include furigana.
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
		// Japanese Notes: This localization should include furigana.
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
		// Notes: used for the header in the achievements menu.
		"Achievements_Name_Uppercase",
		{
			{ ELanguage::English,  "ACHIEVEMENTS" },
			{ ELanguage::Japanese, "実績" },
			{ ELanguage::German,   "ERFOLGE" },
			{ ELanguage::French,   "SUCCÈS" },
			{ ELanguage::Spanish,  "LOGROS" },
			{ ELanguage::Italian,  "OBIETTIVI" }
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
		"Installer_Header_Installer",
		{
			{ ELanguage::English,  "Installer" },
			{ ELanguage::Japanese, "Install" },
			{ ELanguage::German,   "Installation" },
			{ ELanguage::French,   "Installateur" },
			{ ELanguage::Spanish,  "Instalador" },
			{ ELanguage::Italian,  "Installatore" },
		},
	},
	{
		"Installer_Header_Installing",
		{
			{ ELanguage::English,  "Installing" },
			{ ELanguage::Japanese, "Install" },
			{ ELanguage::German,   "Installation" },
			{ ELanguage::French,   "Installation" },
			{ ELanguage::Spanish,  "Instalando" },
			{ ELanguage::Italian,  "Installando" },
		}
	},
	{
		"Installer_Header_Browse",
		{
			{ ELanguage::English,  "Browse" },
			{ ELanguage::Japanese, "ブラウズ" },
			{ ELanguage::German,   "Durchsuchen" },
			{ ELanguage::French,   "Parcourir" },
			{ ELanguage::Spanish,  "Explorar" },
			{ ELanguage::Italian,  "Esplora" },
		}
	},
	{
		// Japanese Notes: This localization should include furigana.
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
		// Japanese Notes: This localization should include furigana.
		"Installer_Page_Introduction",
		{
			{ ELanguage::English,  "Welcome to Shadow Recompiled!\n\nYou'll need an Xbox copy of\nShadow the Hedgehog in order to\nproceed with the installation." },
			{ ELanguage::Japanese, "Shadow Recompiledへようこそ！\nインストールには\n[北米版:ほくべいばん]またはEU[版:ばん]のXbox[用:よう]\n「SHADOW THE HEDGEHOG」が[必要:ひつよう]です" },
			{ ELanguage::German,   "Willkommen zu Shadow Recompiled!\nEs wird eine Xbox Kopie von Shadow the Hedgehog benötigt um mit der Installation fortfahren zu können." },
			{ ELanguage::French,   "Bienvenue sur Shadow Recompiled !\n\nVous aurez besoin d'une copie de Shadow the Hedgehog pour Xbox\npour procéder à l'installation." },
			{ ELanguage::Spanish,  "¡Bienvenido a Shadow Recompiled!\n\nNecesitas una copia de\nShadow the Hedgehog de Xbox\npara continuar con la instalación." },
			{ ELanguage::Italian,  "Benvenuto a Shadow Recompiled!\n\nDovrai avere una copia di\nShadow the Hedgehog per la Xbox\nper proseguire con l'installazione." }
		}
	},
	{
		// Japanese Notes: This localization should include furigana.
		"Installer_Page_SelectGame",
		{
			{ ELanguage::English,  "Add the sources for the game." },
			{ ELanguage::Japanese, "ゲームのソースを[追加:ついか]" },
			{ ELanguage::German,   "Füge die Quellen für das Spiel hinzu." },
			{ ELanguage::French,   "Ajouter les fichiers du jeu." },
			{ ELanguage::Spanish,  "Añade las fuentes para el juego." },
			{ ELanguage::Italian,  "Aggiungi le fonti per il gioco." }
		}
	},
	{
		// Japanese Notes: This localization should include furigana.
		"Installer_Page_NoFileSelected",
		{
			{ ELanguage::English,  "No file selected..." },
			{ ELanguage::Japanese, "ファイルが選択されていません..." },
			{ ELanguage::German,   "Keine Datei ausgewählt..." },
			{ ELanguage::French,   "Aucun fichier sélectionné..." },
			{ ELanguage::Spanish,  "Ningún archivo seleccionado..." },
			{ ELanguage::Italian,  "Nessun file selezionato..." }
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
		"Installer_Page_Verifying",
		{
			{ ELanguage::English,  "Please wait while the disc image is being verified..." },
			{ ELanguage::Japanese, "ディスクイメージの検証[中:ちゅう]はお[待:ま]ち\nください" },
			{ ELanguage::German,   "Bitte warten. Das Disc-Image wird überprüft..." },
			{ ELanguage::French,   "Veuillez patienter pendant la vérification de l'image du disque..." },
			{ ELanguage::Spanish,  "Por favor, espera mientras se verifica la imagen del disco... " },
			{ ELanguage::Italian,  "Attendi mentre l'immagine del disco viene verificata... " }
		}
	},
	{
		// Japanese Notes: This localization should include furigana.
		"Installer_Page_Installing",
		{
			{ ELanguage::English,  "Please wait while the content is being installed..." },
			{ ELanguage::Japanese, "コンテンツのインストール[中:ちゅう]はお[待:ま]ち\nください" },
			{ ELanguage::German,   "Bitte warten. Der Inhalt wird installiert..." },
			{ ELanguage::French,   "Veuillez patienter pendant l'installation du contenu..." },
			{ ELanguage::Spanish,  "Por favor, espera mientras el contenido se instala... " },
			{ ELanguage::Italian,  "Attendi mentre il contenuto viene installato... " }
		}
	},
	{
		// Japanese Notes: This localization should include furigana.
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
		// Japanese Notes: This localization should include furigana.
		"Installer_Page_InstallFailed",
		{
			{ ELanguage::English,  "Installation failed.\n\nAn unknown error occurred." },
			{ ELanguage::Japanese, "インストールに[失敗:しっぱい]しました\n\nエラー： " },
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
		"Installer_Step_Update",
		{
			{ ELanguage::English,  "Update" },
			{ ELanguage::Japanese, "アップデート" },
			{ ELanguage::German,   "Update" },
			{ ELanguage::French,   "Mise à jour" },
			{ ELanguage::Spanish,  "Actualización" },
			{ ELanguage::Italian,  "Aggiornamento" }
		}
	},
	{
		// Japanese Notes: This localization should include furigana.
		"Installer_Step_RequiredSpace",
		{
			{ ELanguage::English,  "Required space: %2.3f GiB" },
			{ ELanguage::Japanese, "[必要:ひつよう]な[容量:ようりょう]: %2.3f GiB" },
			{ ELanguage::German,   "Benötigter Speicherplatz:\n%2.2f GiB" },
			{ ELanguage::French,   "Espace nécessaire :\n%2.3f Gio" },
			{ ELanguage::Spanish,  "Espacio necesario: %2.3f GiB" },
			{ ELanguage::Italian,  "Spazio necessario: %2.3f GiB" }
		}
	},
	{
		// Japanese Notes: This localization should include furigana.
		"Installer_Step_AvailableSpace",
		{
			{ ELanguage::English,  "Available space: %2.3f GiB" },
			{ ELanguage::Japanese, "[使用:しよう][可能:かのう]な[容量:ようりょう]: %2.3f GiB" },
			{ ELanguage::German,   "Verfügbarer Speicherplatz:\n%2.3f GiB" },
			{ ELanguage::French,   "Espace disponible :\n%2.3f Gio" },
			{ ELanguage::Spanish,  "Espacio disponible: %2.3f GiB" },
			{ ELanguage::Italian,  "Spazio disponibile: %2.3f GiB" }
		}
	},
	{
		"Installer_Button_Next", // back already exist in common
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
		"Installer_Button_AddFiles",
		{
			{ ELanguage::English,  "Add Files" },
			{ ELanguage::Japanese, "ファイルを追加" },
			{ ELanguage::German,   "Dateien hinzufügen" },
			{ ELanguage::French,   "Ajouter des fichiers" },
			{ ELanguage::Spanish,  "Añadir archivos" },
			{ ELanguage::Italian,  "Aggiungi file" }
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
		// Notes: message appears when using the "Add Files" option and choosing any file that is not an Xbox game dump.
		// Japanese Notes: This localization should include furigana.
		"Installer_Message_InvalidFileSelected",
		{
			{ ELanguage::English,  "The selected file is not a valid Shadow the Hedgehog Xbox ISO.\nPlease provide a valid ISO file." },
			{ ELanguage::Japanese, "" },
			{ ELanguage::German,   "" },
			{ ELanguage::French,   "Le fichier sélectionné n'est pas une image valide de Shadow the Hedgehog pour Xbox.\nVeuillez fournir un fichier ISO valide." },
			{ ELanguage::Spanish,  "" },
			{ ELanguage::Italian,  "" }
		}
	},
	{
		// Notes: message appears when clicking the "Add Files" option for the first time.
		// Japanese Notes: This localization should include furigana.
		"Installer_Message_FilePickerTutorial",
		{
			{ ELanguage::English,  "Select a digital dump with\ncontent from the game.\n\nThese files can be obtained from\nyour Xbox hard drive by\nfollowing the instructions on\nthe GitHub page.\n\nFor choosing a folder with extracted\nand unmodified game files, use\nthe \"Add Folder\" option instead." },
			{ ELanguage::Japanese, "ゲームのコンテンツを[含:ふく]む デジタルダンプを[選択:せんたく]してください\n\nこれらのファイルは GitHubページの[指示:しじ]に[従:したが]って\nXboxハードドライブから[取得:しゅとく]できます\n\n[抽出:ちゅうしゅつ]された[変更:へんこう]されていない\nゲームファイルを[含:ふく]むフォルダーを[選択:せんたく]するには\n[代:か]わりに「フォルダの[追加:ついか]」オプションを[使用:しよう]してください" },
			{ ELanguage::German,   "Wähle einen digitalen Dump von dem Spiel.\n\nDie Dateien können über die Festplatte deiner\nXbox erlangt werden.\nFolge hierfür den Anweisungen auf der GitHub Seite.\n\nUm einen Ordner mit unmodifizierten Spieldateien auszuwählen, benutze die \"Ordner Hinzufügen\" Option stattdessen." },
			{ ELanguage::French,   "Sélectionnez une copie\ndématérialisée avec le contenu du\njeu de base.\n\nCes fichiers peuvent être obtenus\nà partir du disque dur de votre\nXbox en suivant les\ninstructions de la page GitHub.\n\nPour choisir un dossier contenant\nles fichiers de jeu extraits et\nnon modifiés, utilisez plutôt\nl'option \"Ajouter un dossier\"." },
			{ ELanguage::Spanish,  "Selecciona una copia digital\ncon contenido del juego.\n\nPuedes obtener los archivos\nde tu disco duro de Xbox\nsiguiendo las instrucciones de\nla página de GitHub.\n\nPara elegir una carpeta con\narchivos extraídos sin modificar,\nutiliza la opción \"Añadir Carpeta\"." },
			{ ELanguage::Italian,  "Seleziona una copia digitale\ncon i contenuti del gioco.\n\nQuesti file possono essere ottenuti\ndall'hard drive della tua Xbox\nseguendo le istruzioni\nsulla pagina GitHub.\n\nPer selezionare una cartella\ncon file estratti e non modificati\nusa l'opzione \"Aggiungi cartella\"." }
		}
	},
	{
		// Notes: message appears when clicking the "Add Folder" option for the first time.
		// Japanese Notes: This localization should include furigana.
		"Installer_Message_FolderPickerTutorial",
		{
			{ ELanguage::English,  "Select a folder that contains the\nunmodified files that have been\nextracted from the game.\n\nThese files can be obtained from\nyour Xbox hard drive by\nfollowing the instructions on\nthe GitHub page.\n\nFor choosing a digital dump,\nuse the \"Add Files\" option instead." },
			{ ELanguage::Japanese, "ゲームから[抽出:ちゅうしゅつ]された[変更:へんこう]されていない\nファイルを含むフォルダを[選択:せんたく]してください\n\nこれらのファイルは GitHubページの[指示:しじ]に[従:したが]って\nXboxハードドライブから[取得:しゅとく]できます\n\nデジタルダンプを[選択:せんたく]するには\n[代:か]わりに「ファイルの[追加:ついか]」オプションを[使用:しよう]してください" },
			{ ELanguage::German,   "Wähle einen Ordner, der unmodifizierte Dateien, die vom Spiel extrahiert wurden enthält.\n\nDie Dateien können über die Festplatte deiner\nXbox erlangt werden.\nFolge hierfür den Anweisungen auf der GitHub Seite.\n\nUm einen digitalen Dump auszuwählen, benutze die \"Datei Hinzufügen\" Option stattdessen." },
			{ ELanguage::French,   "Sélectionnez un dossier contenant\nles fichiers extraits du jeu de\nbase.\n\nCes fichiers peuvent être obtenus\nà partir du disque dur de votre\nXbox en suivant les\ninstructions de la page GitHub.\n\nPour choisir une copie\ndématérialisée, utilisez plutôt\nl'option \"Ajouter des fichiers\"." },
			{ ELanguage::Spanish,  "Selecciona una carpeta que\ncontenga los archivos sin\nmodificar extraídos del juego.\n\nPuedes obtener los archivos\nde tu disco duro de Xbox\nsiguiendo las instrucciones de\nla página de GitHub.\n\nPara elegir una copia digital,\nutiliza la opción \"Añadir Archivos\"." },
			{ ELanguage::Italian,  "Seleziona una cartella che contiene\ni file non modificati che sono stati\nestratti dal gioco.\n\nQuesti file possono essere ottenuti\ndall'hard drive della tua Xbox\nseguendo le istruzioni\nsulla pagina GitHub.\n\nPer selezionare una copia digitale\nusa l'opzione \"Aggiungi file\"." }
		}
	},
	{
		// Notes: message appears when pressing B at the title screen.
		// Japanese Notes: This localization should include furigana.
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
		// Japanese Notes: This localization should include furigana.
		"Title_Message_CreateSave",
		{
			{ ELanguage::English,  "There is no Game Data on your PC.\nSHADOW THE HEDGEHOG™\nsupports an autosave feature.\nDo you want to create Game Data?" },
			{ ELanguage::Japanese, "" },
			{ ELanguage::German,   "" },
			{ ELanguage::French,   "Pas de données de jeu sur ce PC.\nSHADOW THE HEDGEHOG™\ndispose d'un système de sauvegarde automatique.\nCréer des données de jeu sur\nce PC?" },
			{ ELanguage::Spanish,  "" },
			{ ELanguage::Italian,  "" }
		}
	},
	{
		// Notes: message appears when creating a save file at the title screen.
		// Note, the game exclusively use "Game Data" when referring to save file, please re use this for consistency.
		// Japanese Notes: This localization should include furigana.
		"Title_Message_CreateSaveFailed",
		{
			{ ELanguage::English,  "Start the game without creating\nGame Data.\nProgress of the game will not be saved." },
			{ ELanguage::Japanese, "" },
			{ ELanguage::German,   "" },
			{ ELanguage::French,   "Démarrer le jeu sans créer de données du jeu.\nLa progression du jeu ne sera\npas sauvegardée." },
			{ ELanguage::Spanish,  "" },
			{ ELanguage::Italian,  "" }
		}
	},
	{
		// Notes: message appears when SYS-DATA is corrupted (mismatching file size) upon pressing start at the title screen.
		// To make this occur, open the file in any editor and just remove a large chunk of data.
		"Title_Message_SaveDataCorrupt",
		{
			{ ELanguage::English,  "The save file appears to be\ncorrupted and cannot be loaded." },
			{ ELanguage::Japanese, "ゲームデータの読み込みに失敗しました。\nこのまま続けるとゲームデータをセーブすることはできません" },
			{ ELanguage::German,   "Diese Speicherdatei ist beschädigt\nund kann nicht geladen werden." },
			{ ELanguage::French,   "Le fichier de sauvegarde semble être\nendommagé et ne peut être chargé." },
			{ ELanguage::Spanish,  "El archivo parece estar dañado\ny no se puede cargar." },
			{ ELanguage::Italian,  "I file di salvataggio sembrano danneggiati\ne non possono essere caricati." }
		}
	},
	{
		// Notes: message appears when creating a save file at the title screen.
		// Note, the game exclusively use "Game Data" when referring to save file, please re use this for consistency.
		// Japanese Notes: This localization should include furigana.
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
		// Japanese Notes: This localization should include furigana.
		"Title_Message_ContinueWithoutSave",
		{
			{ ELanguage::English,  "Continue without saving ?" }, // there is a space added before the question mark to match the original game text, even in english.
			{ ELanguage::Japanese, "Continue without saving ?" },
			{ ELanguage::German,   "Continue without saving ?" },
			{ ELanguage::French,   "Continuez sans sauvegarder ?" },
			{ ELanguage::Spanish,  "Continue without saving ?" },
			{ ELanguage::Italian,  "Continue without saving ?" },
		}
	},
	{
		// Notes: message appears when ACH-DATA is corrupted (mismatching file size, bad signature, incorrect version or invalid checksum) upon pressing start at the title screen.
		// To make this occur, open the file in any editor and just remove a large chunk of data.
		// Japanese Notes: This localization should include furigana.
		"Title_Message_AchievementDataCorrupt",
		{
			{ ELanguage::English,  "The achievement data appears to be\ncorrupted and cannot be loaded.\n\nProceeding from this point will\nclear your achievement data." },
			{ ELanguage::Japanese, "[実績:じっせき]データが[破損:はそん]しているため\n[読:よ]み[込:こ]むことができません\n\nこの[先:さき]に[進:すす]むと[実績:じっせき]データが\n[消去:しょうきょ]されます" },
			{ ELanguage::German,   "Die Erfolgsdaten sind möglicherweise\nfehlerhaft und können nicht\ngeladen werden.\n\nDurch das Fortfahren werden\ndeine bisherigen Erfolgsdaten gelöscht." },
			{ ELanguage::French,   "Les données des succès semblent être\nendommagées et ne peuvent être\nchargées.\n\nSi vous continuez, vos données\nseront écrasées." },
			{ ELanguage::Spanish,  "Los datos de logros parecen estar\ncorruptos y no pueden cargarse.\n\nContinuar a partir de este punto\neliminará los datos de logros." },
			{ ELanguage::Italian,  "I file degli obiettivi sembrano danneggiati\ne non possono essere caricati.\n\nSe prosegui da questo punto\ni tuoi obiettivi verranno cancellati." }
		}
	},
	{
		// Notes: message appears when ACH-DATA cannot be loaded upon pressing start at the title screen.
		// To make this occur, lock the ACH-DATA file using an external program so that it cannot be accessed by the game.
		// Japanese Notes: This localization should include furigana.
		"Title_Message_AchievementDataIOError",
		{
			{ ELanguage::English,  "The achievement data could not be loaded.\nYour achievements will not be saved." },
			{ ELanguage::Japanese, "[実績:じっせき]データを[読:よ]み[込:こ]めませんでした\n[実績:じっせき]は[保存:ほぞん]されません。" },
			{ ELanguage::German,   "Die Erfolgsdaten konnten nicht geladen werden.\nDeine Erfolge werden nicht gespeichert." },
			{ ELanguage::French,   "Les données des succès ne\npeuvent être chargées.\nVos succès ne seront pas\nsauvegardés." },
			{ ELanguage::Spanish,  "Los datos de logros no pueden cargarse.\nTus logros no serán guardados." },
			{ ELanguage::Italian,  "I file degli obiettivi non possono essere caricati.\nI tuoi obiettivi non verranno salvati." }
		}
	},
	{
		// Japanese Notes: This localization should include furigana.
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
			{ ELanguage::German,  "Die Installation wurde überprüft.\n\nAlle Dateien scheinen korrekt zu sein.\n\nDas Spiel wird nun geschlossen. Entferne die Startoption, um das Spiel zu spielen." },
			{ ELanguage::French,  "La vérification de l'installation est terminée.\n\nTous les fichiers semblent corrects.\n\nL'application va maintenant se fermer. Retirez l'argument de lancement pour pouvoir lancer le jeu." },
			{ ELanguage::Spanish,  "La verificación de la instalación ha terminado.\n\nTodos los archivos parecen correctos.\n\nEl juego se cerrará ahora. Elimina el argumento de lanzamiento para jugar al juego." },
			{ ELanguage::Italian,  "La verifica dei file d'installazione è terminata.\n\nTutti i file sembrano corretti.\n\nIl gioco si chiuderà. Rimuovi l'argomento di avvio per poter giocare." }
		}
	},
	{
		"IntegrityCheck_Failed",
		{
			{ ELanguage::English,  "Installation check has failed.\n\nError: %s\n\nThe game will now close. Try reinstalling the game by using the --install launch argument." },
			{ ELanguage::Japanese, "インストールチェックに失敗しました\n\nエラー：%s\n\nゲームは終了します、--install 起動引数を使用してゲームを再インストールしてください" },
			{ ELanguage::German,  "Die Installationsprüfung ist fehlgeschlagen.\n\nFehler: %s\n\nDas Spiel wird nun geschlossen. Versuche das Spiel durch Verwendung der Startoption --install neu zu installieren." },
			{ ELanguage::French,  "La vérification de l'installation a échoué.\n\nErreur : %s\n\nL'application va maintenant se fermer. Essayez de réinstaller le jeu en utilisant l'argument de lancement --install." },
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
			{ ELanguage::Spanish, "Sí" },
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
