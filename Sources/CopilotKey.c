
// Copilot Key+
// © Captain FLAM - 2026 - Licence MIT

// Touche Copilot matérielle dédiée (HID Consumer Page 0x0C, Usage 0x0D8,
// Windows 11 >= 23H2) : contrairement aux salves de scancodes (page HID 0x07)
// captées par WH_KEYBOARD_LL, cette touche n'émet qu'un usage de la page
// Consumer - invisible pour ce hook. Repli dédié : quand la capture scancode
// classique à l'install ne trouve rien (voir CaptureOneHidAttempt), on écoute
// en Raw Input et on apprend dynamiquement l'usage réellement vu (jamais figé
// en dur sur 0x0D8, qui ne sert que de libellé d'affichage connu - un usage
// différent serait appris et fonctionnerait quand même, juste affiché en hex).
//
// Limites connues, non vérifiées sur matériel réel :
//  - Si Windows intercepte la touche avant même Raw Input (traitement shell
//    prioritaire), aucun WM_INPUT n'arrive : "Aucune touche détectée" en
//    boucle côté -config, sans aucune ligne HID (rouge ou DEBUG_LOG). Pas de
//    repli prévu pour ce cas.
//  - Le format exact du rapport Consumer Control (array vs bitmap) varie
//    selon l'OEM ; HidP_GetUsages est censé gérer les deux.


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <hidsdi.h>
#include <hidpi.h>
#include <hidusage.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// #define LOG 1

#ifdef LOG
	#define DEBUG_LOG(fmt, ...) printf("[%lu] " fmt, GetTickCount(), __VA_ARGS__)
#else
	#define DEBUG_LOG(fmt, ...) // Ne fait rien si LOG n'est pas défini
#endif

typedef enum { LANG_EN, LANG_FR } Lang;
static Lang g_lang = LANG_EN;

// printf bilingue : imprime "fr" ou "en" selon la langue choisie en début
// d'installation ; les deux chaînes doivent avoir les mêmes spécificateurs de
// format (seul le texte change).
static void Say(const char* fr, const char* en, ...)
{
	va_list args;
	va_start(args, en);
	vprintf((g_lang == LANG_EN) ? en : fr, args);
	va_end(args);
}

// Scancodes
#define K_LCTRL  0x1D	// Left CTRL
#define K_LSHIFT 0x2A	// Left SHIFT
#define K_RSHIFT 0x36	// Right SHIFT
#define K_LALT   0x38	// Left ALT
#define K_LWIN   0xE05B	// Left WIN
#define K_F23    0x6E	// F23
#define K_F24    0x76	// F24
#define K_APPS   0xE05D	// Menu contextuel
// (EXTENDED)
#define K_RCTRL 0xE01D	// Right CTRL
#define K_RALT  0xE038	// Right ALT
#define K_RWIN  0xE05C	// Right WIN
#define K_RIGHT 0xE04D	// Right Arrow
#define K_LEFT  0xE04B	// Left Arrow
#define K_UP    0xE048	// Up Arrow
#define K_DOWN  0xE050	// Down Arrow
#define K_HOME  0xE047	// Home
#define K_END   0xE04F	// End
#define K_PGUP  0xE049	// Page Up
#define K_PGDN  0xE051	// Page Down

// Table de référence historique (v1) : la combinaison réelle est désormais apprise
// dynamiquement via "CopilotKey+.exe -config" (voir Signature dans le registre),
// plus aucune logique du programme ne dépend de cette table.

// | Combinaison hardware | OEM connus         |
// | -------------------- | ------------------ |
// | Win + C              | (Microsoft), Dell  | Surface (anciens), Dell 2023, HP early Copilot
// | Win + Shift + F23    | Lenovo, Asus, Acer | Lenovo ThinkPad (récents), Asus Zenbook Copilot, Acer Swift AI
// | Win + Shift + F24    | HP, MSI            | HP EliteBook, certains MSI Business
// | Win + Ctrl + F23     | Dell, HP           | Dell Latitude (récents), HP ProBook
// | Ctrl + Shift + F23   | Lenovo             | Lenovo non-Copilot+ (fallback assistant key)
// | Shift + F23          | Asus, Huawei       | Asus VivoBook, Huawei MateBook
// | Win + F24            | HP                 |
// | Ctrl + F24           | ????               |
// | Shift + F24          | ????               |

#define REG_KEY "Software\\CopilotKey+"

#define MAX_SIG 6
#define PENDING_WINDOW_MS_DEFAULT 50
#define PENDING_WINDOW_MS_MIN 25
#define PENDING_WINDOW_MS_MAX 200
#define CAPTURE_QUIET_MS 700
#define CAPTURE_TIMEOUT_MS 5000

// Sur certains claviers (notamment la touche Copilot HID dédiée), le matériel
// semble relâcher la salve automatiquement au bout de quelques centaines de ms,
// indépendamment de la durée d'appui réelle sur la touche physique. Cette
// fenêtre de grâce laisse le temps d'appuyer sur une flèche juste après, même
// si la salve physique est déjà terminée. Sur d'autres claviers (combo scancode
// classique avec autorépétition, ex. Win+Shift+F23), ce quirk n'existe pas :
// burstState suit fidèlement l'état physique réel, et la fenêtre de grâce n'a
// alors aucune utilité (elle ne fait que prolonger artificiellement le remap
// flèches après un relâchement volontaire). D'où la calibration par -config
// (voir CalibrateArrowGrace/registre "Grace") : ARROW_GRACE_MS_DEFAULT ne sert
// que de repli pour les installations non recalibrées (mise à jour sans
// relancer -config) ou pour le chemin HID, non testé par la calibration.
#define ARROW_GRACE_MS_DEFAULT 250
#define ARROW_GRACE_MS_MIN 100
#define ARROW_GRACE_MS_MAX 1500
// Durée sous laquelle un relâchement vu pendant CalibrateArrowGrace est
// considéré comme un auto-release matériel (et non un relâchement volontaire
// de l'utilisateur en avance sur la consigne) : voir CalibrateArrowGrace.
#define ARROW_GRACE_CALIB_HOLD_MS 2500
#define ARROW_GRACE_CALIB_THRESHOLD_MS 900
#define ARROW_GRACE_CALIB_MARGIN_MS 300

// Fenêtre cachée de contrôle : permet à "CopilotKey+.exe -quit" de retrouver
// l'instance résidente et de lui demander de s'arrêter proprement.
#define QUIT_WINDOW_CLASS "CopilotKeyPlusCtl"

// Couleurs ANSI pour la console d'installation
#define FC_END    "\033[0m"
#define FC_RED    "\033[0;31m"
#define FC_GREEN  "\033[0;32m"
#define FC_CYAN   "\033[0;36m"
#define FC_YELLOW "\033[0;33m"
#define FC_DARKGRAY "\033[1;30m"

// Touche Copilot (CTRL droit par défaut)
int CopilotKey = VK_RCONTROL;

// Active/désactive le remap flèches -> Home/End/PgUp/PgDn quand Copilot est
// tenu (activé par défaut). Uniquement déclenché par la touche Copilot elle-même.
BOOL ArrowsEnabled = TRUE;

// Flèches remap
typedef struct
{
	DWORD vkArrow;  // touche physique flèche
	WORD scanCode;  // scancode de la touche cible (Home, End, PgUp, PgDn)
	BOOL down;      // is key already down ?
} ArrowMap;

ArrowMap arrowMaps[4];

// Signature Copilot apprise (scancodes, 0xE0xx si étendu)
typedef struct
{
	WORD code;
	BOOL down;           // état physique courant
	BOOL passedThrough;  // déjà transmis normalement à l'OS (touche réelle, pas la salve)
	BOOL modifier;        // Shift/Ctrl/Alt "sûrs" : voir IsSafeModifierScanCode
	BOOL neutralized;     // ShiftUp compensateur déjà injecté (voir TryConfirmBurst/EndActiveCombo) :
	                      // tant que c'est le cas, les répétitions de "bas" matérielles de la salve
	                      // (voir KeyboardProc) ne doivent plus être retransmises à l'OS.
	BOOL preHeld;         // Modificateur déjà physiquement tenu par l'utilisateur AVANT le début de
	                      // cette salve (front montant vu alors que burstState valait BURST_IDLE) :
	                      // impossible à distinguer d'un artefact matériel une fois la salve démarrée
	                      // (même scancode), mais un appui antérieur au tout premier signal de la
	                      // salve ne peut être QUE l'utilisateur. Dans ce cas, TryConfirmBurst ne le
	                      // neutralise pas : il continue d'être transmis normalement à l'OS, ce qui
	                      // permet des combos type Copilot+Maj+Flèche quand Maj fait partie de la
	                      // signature de cette machine.
} SigSlot;

static SigSlot sig[MAX_SIG];
static int     sigCount = 0;

// Signature alternative : touche Copilot matérielle dédiée (voir commentaire
// en tête de fichier). Mutuellement exclusive avec sig[]/sigCount : quand
// g_sigIsHid est vrai, sigCount reste à 0 et la salve de scancodes ne fait
// rien (KeyboardProc), toute la détection passe par Raw Input (WM_INPUT).
static BOOL g_sigIsHid  = FALSE;
static WORD g_hidUsage  = 0;
static BOOL g_hidKeyDown = FALSE;

// Shift/Ctrl/Alt : scancodes fixes et universels (norme PS/2 Set 1), identiques
// sur tout clavier quel que soit le fabricant - aucune ambiguïté à lever par
// capture, contrairement à la salve Copilot elle-même qui varie selon l'OEM.
// Ces touches sont extrêmement utilisées seules (frappe rapide, jeu) : quand
// l'une d'elles fait partie de la signature, on ne la retient jamais en attente
// de confirmation (voir KeyboardProc) pour ne jamais ajouter de latence sur un
// usage normal. Win est volontairement exclue de cette liste : la laisser fuir
// seule vers l'OS avant confirmation risquerait de déclencher le menu Démarrer.
static BOOL IsSafeModifierScanCode(WORD code)
{
	switch (code)
	{
		case K_LCTRL: case K_RCTRL:
		case K_LSHIFT: case K_RSHIFT:
		case K_LALT: case K_RALT:
			return TRUE;
		default:
			return FALSE;
	}
}


typedef enum { BURST_IDLE, BURST_PENDING, BURST_ACTIVE } BurstState;
static BurstState  burstState = BURST_IDLE;
static UINT_PTR    g_pendingTimerId = 0;
static DWORD       g_pendingWindowMs = PENDING_WINDOW_MS_DEFAULT;
static DWORD       g_arrowGraceMs = ARROW_GRACE_MS_DEFAULT;  // voir "Grace" dans LoadRegistrySettings / CalibrateArrowGrace

static BOOL  copilotDown = FALSE;
static DWORD g_burstReleaseTick = 0;  // GetTickCount() du dernier relâchement naturel de la salve

// Fonctions utilitaires
static inline BOOL IsKeyDown(WPARAM wParam) { return wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN; }
static inline BOOL IsKeyUp(WPARAM wParam)   { return wParam == WM_KEYUP   || wParam == WM_SYSKEYUP; }

static inline WORD ScanCodeOf(const KBDLLHOOKSTRUCT* k)
{
	WORD code = (WORD)k->scanCode;
	if (k->flags & LLKHF_EXTENDED) code |= 0xE000;
	return code;
}

static int FindSigSlot(WORD code)
{
	for (int i = 0; i < sigCount; i++) if (sig[i].code == code) return i;
	return -1;
}

// Extrait le premier usage non nul de la page HID Consumer (0x0C) présent
// dans un message WM_INPUT, ou 0 si absent/non pertinent (autre page, autre
// type de périphérique). Utilisé à la fois par la capture -config
// (CaptureOneHidAttempt) et par le résident (HiddenWndProc) pour la touche
// Copilot matérielle dédiée (voir commentaire en tête de fichier).
static WORD ParseConsumerUsageFromRawInput(HRAWINPUT hRawInput)
{
	UINT size = 0;
	if (GetRawInputData(hRawInput, RID_INPUT, NULL, &size, sizeof(RAWINPUTHEADER)) != 0 || size == 0)
		return 0;

	BYTE stackBuf[256];
	BYTE* buf = (size <= sizeof(stackBuf)) ? stackBuf : (BYTE*)malloc(size);
	if (! buf) return 0;

	WORD result = 0;

	if (GetRawInputData(hRawInput, RID_INPUT, buf, &size, sizeof(RAWINPUTHEADER)) == size)
	{
		RAWINPUT* ri = (RAWINPUT*)buf;
		if (ri->header.dwType == RIM_TYPEHID && ri->data.hid.dwCount > 0)
		{
			UINT ppSize = 0;
			GetRawInputDeviceInfoW(ri->header.hDevice, RIDI_PREPARSEDDATA, NULL, &ppSize);
			if (ppSize > 0)
			{
				PHIDP_PREPARSED_DATA preparsed = (PHIDP_PREPARSED_DATA)malloc(ppSize);
				if (preparsed)
				{
					if (GetRawInputDeviceInfoW(ri->header.hDevice, RIDI_PREPARSEDDATA, preparsed, &ppSize) == ppSize)
					{
						USAGE usageList[16];
						ULONG usageLen = ARRAYSIZE(usageList);
						if (HidP_GetUsages(HidP_Input, HID_USAGE_PAGE_CONSUMER, 0, usageList, &usageLen,
						                   preparsed, (PCHAR)ri->data.hid.bRawData, ri->data.hid.dwSizeHid) == HIDP_STATUS_SUCCESS)
						{
							for (ULONG i = 0; i < usageLen; i++)
							{
								if (usageList[i] != 0) { result = (WORD)usageList[i]; break; }
							}
						}
					}
					free(preparsed);
				}
			}
		}
	}

	if (buf != stackBuf) free(buf);
	return result;
}

static void ParseSignature(const char* s)
{
	char buf[256];
	strncpy_s(buf, sizeof(buf), s, _TRUNCATE);

	char* ctx = NULL;
	char* tok = strtok_s(buf, ",", &ctx);
	while (tok && sigCount < MAX_SIG)
	{
		sig[sigCount].code = (WORD)strtol(tok, NULL, 16);
		sig[sigCount].down = FALSE;
		sig[sigCount].passedThrough = FALSE;
		sig[sigCount].modifier = IsSafeModifierScanCode(sig[sigCount].code);
		sig[sigCount].neutralized = FALSE;
		sig[sigCount].preHeld = FALSE;
		sigCount++;
		tok = strtok_s(NULL, ",", &ctx);
	}
}

static void SerializeSignature(char* out, size_t outSize)
{
	out[0] = '\0';
	for (int i = 0; i < sigCount; i++)
	{
		char tmp[16];
		sprintf_s(tmp, sizeof(tmp), "%s%04X", (i > 0) ? "," : "", sig[i].code);
		strcat_s(out, outSize, tmp);
	}
}

void LoadRegistrySettings()
{
	HKEY hKey;
	DWORD size;
	DWORD value = sizeof(DWORD);
	char sigBuf[256];
	char hidBuf[16];

	sigCount = 0;
	g_sigIsHid = FALSE;
	g_hidUsage = 0;

	if (RegOpenKeyExA(HKEY_CURRENT_USER, REG_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		// Mode Copilot
		size = sizeof(DWORD);
		if (RegQueryValueExA(hKey, "Mode", NULL, NULL, (LPBYTE)&value, &size) == ERROR_SUCCESS)
		{
			switch (value) {
				case 0: CopilotKey = 0; break;				// désactivé
				case 1: CopilotKey = VK_RCONTROL; break;	// CTRL droit
				case 2: CopilotKey = VK_APPS; break;		// Menu contextuel
				default: CopilotKey = VK_RCONTROL;
			}
		}

		// Effet flèches (Home/End/PgUp/PgDn) quand Copilot est tenu
		size = sizeof(DWORD);
		if (RegQueryValueExA(hKey, "Arrows", NULL, NULL, (LPBYTE)&value, &size) == ERROR_SUCCESS)
		{
			ArrowsEnabled = (value != 0);
		}

		// Touche Copilot matérielle dédiée (voir commentaire en tête de fichier) :
		// présence de cette valeur = signature HID Consumer, exclusive de "Signature".
		size = sizeof(hidBuf);
		if (RegQueryValueExA(hKey, "HIDconsumer", NULL, NULL, (LPBYTE)hidBuf, &size) == ERROR_SUCCESS)
		{
			g_hidUsage = (WORD)strtol(hidBuf, NULL, 16);
			g_sigIsHid = TRUE;
		}
		else
		{
			// Signature Copilot apprise via -config
			size = sizeof(sigBuf);
			if (RegQueryValueExA(hKey, "Signature", NULL, NULL, (LPBYTE)sigBuf, &size) == ERROR_SUCCESS)
			{
				ParseSignature(sigBuf);
			}
		}

		// Fenêtre de confirmation de la salve, mesurée sur le matériel via -config
		size = sizeof(DWORD);
		if (RegQueryValueExA(hKey, "Burst", NULL, NULL, (LPBYTE)&value, &size) == ERROR_SUCCESS && value > 0)
		{
			g_pendingWindowMs = value;
		}

		// Fenêtre de grâce flèches, calibrée sur le matériel via -config (voir
		// CalibrateArrowGrace) ; ARROW_GRACE_MS_DEFAULT si absente (mise à jour
		// sans relancer -config, ou chemin HID non calibré).
		size = sizeof(DWORD);
		if (RegQueryValueExA(hKey, "Grace", NULL, NULL, (LPBYTE)&value, &size) == ERROR_SUCCESS && value > 0)
		{
			g_arrowGraceMs = value;
		}

		RegCloseKey(hKey);
	}

	if (! g_sigIsHid && sigCount == 0)
	{
		// Repli par défaut : combinaison la plus répandue (Lenovo/Asus/Acer), tant
		// qu'aucune signature n'a été apprise via -config. Ne s'applique qu'au
		// format scancode, pas au mode HID Consumer.
		sig[0].code = K_F23;
		sig[0].down = FALSE;
		sig[0].passedThrough = FALSE;
		sig[0].modifier = FALSE;
		sig[0].neutralized = FALSE;
		sig[0].preHeld = FALSE;
		sigCount = 1;
	}

	arrowMaps[0] = (ArrowMap){VK_LEFT,  (WORD)MapVirtualKeyA(VK_HOME,  MAPVK_VK_TO_VSC), FALSE};  // HOME
	arrowMaps[1] = (ArrowMap){VK_RIGHT, (WORD)MapVirtualKeyA(VK_END,   MAPVK_VK_TO_VSC), FALSE};  // END
	arrowMaps[2] = (ArrowMap){VK_UP,    (WORD)MapVirtualKeyA(VK_PRIOR, MAPVK_VK_TO_VSC), FALSE};  // Page Up
	arrowMaps[3] = (ArrowMap){VK_DOWN,  (WORD)MapVirtualKeyA(VK_NEXT,  MAPVK_VK_TO_VSC), FALSE};  // Page Down
}

void SendCopilotKey(BOOL down)
{
	if (CopilotKey == 0) return;

	INPUT input = {0};
	input.type = INPUT_KEYBOARD;

	if (CopilotKey == VK_RCONTROL) {
		input.ki.wScan = 0x1D; // scancode Right CTRL
		input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_EXTENDEDKEY;
	}
	else {
		input.ki.wVk = (WORD)CopilotKey;
	}

	if (! down) input.ki.dwFlags |= KEYEVENTF_KEYUP;
	SendInput(1, &input, sizeof(INPUT));
	copilotDown = down;
}

static void InjectScanKey(WORD code, BOOL down)
{
	INPUT input = {0};
	input.type = INPUT_KEYBOARD;
	input.ki.wScan = (BYTE)(code & 0xFF);
	input.ki.dwFlags = KEYEVENTF_SCANCODE;
	if (code & 0xFF00) input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
	if (! down) input.ki.dwFlags |= KEYEVENTF_KEYUP;
	SendInput(1, &input, sizeof(INPUT));
}

void SendArrow(int i, BOOL down)
{
	INPUT input = {0};
	input.type = INPUT_KEYBOARD;
	input.ki.wScan = arrowMaps[i].scanCode;
	input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_EXTENDEDKEY;
	if (! down) input.ki.dwFlags |= KEYEVENTF_KEYUP;
	SendInput(1, &input, sizeof(INPUT));
	arrowMaps[i].down = down;
}

// Une touche de la signature attendait la confirmation du reste de la salve
// (fenêtre PENDING_WINDOW_MS) mais celle-ci ne s'est pas complétée : on rejoue
// telles quelles toutes les touches actuellement avalées, elles deviennent des
// touches "réelles" normalement transmises tant qu'elles restent enfoncées.
static void AbortPendingBurst(void)
{
	if (g_pendingTimerId) { KillTimer(NULL, g_pendingTimerId); g_pendingTimerId = 0; }

	for (int i = 0; i < sigCount; i++)
	{
		if (sig[i].modifier) continue; // jamais retenues (voir IsSafeModifierScanCode), rien à rejouer
		if (sig[i].down && ! sig[i].passedThrough)
		{
			InjectScanKey(sig[i].code, TRUE);
			sig[i].passedThrough = TRUE;
		}
	}
	burstState = BURST_IDLE;
}

static void CALLBACK PendingTimeoutProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	UNREFERENCED_PARAMETER(hwnd);
	UNREFERENCED_PARAMETER(uMsg);
	UNREFERENCED_PARAMETER(idEvent);
	UNREFERENCED_PARAMETER(dwTime);

	if (burstState == BURST_PENDING)
	{
		DEBUG_LOG("Pending confirmation timed out, replaying held keys\n");
		AbortPendingBurst();
	}
}

// Réévalue si la salve retenue est désormais complète (tous les slots de la
// signature "down") et confirme la combinaison si c'est le cas. Appelée à
// chaque touche de la signature qui passe à l'état bas, qu'il s'agisse d'une
// touche retenue (Win/distinctive) ou d'un modificateur "sûr" transmis en
// direct - la signature peut se compléter dans n'importe quel ordre (touche
// distinctive avant ou après Shift/Ctrl/Alt, modificateurs entre eux, etc.).
static void TryConfirmBurst(void)
{
	if (burstState != BURST_PENDING) return;

	for (int i = 0; i < sigCount; i++) if (! sig[i].down) return;

	if (g_pendingTimerId) { KillTimer(NULL, g_pendingTimerId); g_pendingTimerId = 0; }

	// Les modificateurs "sûrs" ont déjà été transmis en direct : on les
	// neutralise ponctuellement (touche haut synthétique) pour que l'appli au
	// premier plan reçoive une touche Copilot propre (ex. RightCtrl seul, pas
	// Shift+RightCtrl). Restaurés dans EndActiveCombo si encore physiquement
	// tenus à la fin. Exception : un modificateur déjà tenu AVANT le début de
	// cette salve (preHeld) est forcément un choix délibéré de l'utilisateur
	// (impossible pour la salve elle-même d'avoir démarré avant son propre
	// premier signal) - on le laisse alors transmis normalement, ce qui
	// permet Copilot+Maj+Flèche même quand Maj fait partie de la signature.
	if (CopilotKey != 0)
	{
		for (int i = 0; i < sigCount; i++)
			if (sig[i].modifier && sig[i].down && ! sig[i].preHeld) { InjectScanKey(sig[i].code, FALSE); sig[i].neutralized = TRUE; }
	}

	burstState = BURST_ACTIVE;
	DEBUG_LOG("Copilot combo confirmed\n");
	SendCopilotKey(TRUE);
}

// Termine une combinaison Copilot active (relâchement, quelle que soit la
// touche - retenue ou modificateur "sûr" - qui l'a déclenché) : relâche la
// touche synthétique, puis restaure vers l'OS l'état réel des modificateurs
// encore physiquement tenus (neutralisés le temps de la combinaison, voir le
// commentaire "complete" dans KeyboardProc).
static void EndActiveCombo(void)
{
	DEBUG_LOG("Copilot combo released\n");
	SendCopilotKey(FALSE);

	if (CopilotKey != 0)
	{
		for (int i = 0; i < sigCount; i++)
		{
			if (! sig[i].modifier) continue;
			if (sig[i].down && sig[i].neutralized) InjectScanKey(sig[i].code, TRUE);
			sig[i].neutralized = FALSE;
		}
	}

	burstState = BURST_IDLE;
	g_burstReleaseTick = GetTickCount();
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode != HC_ACTION) return CallNextHookEx(NULL, nCode, wParam, lParam);

	KBDLLHOOKSTRUCT* k = (KBDLLHOOKSTRUCT*)lParam;

	// Log des touches interceptées
	DEBUG_LOG("Key intercepted: vkCode=0x%X, flags=0x%X, wParam=0x%X\n", k->vkCode, k->flags, (unsigned int)wParam);

	// Don't process injected keys by "SendInput" in this program
	if (k->flags & LLKHF_INJECTED)
	{
		DEBUG_LOG("Injected key ignored: vkCode=0x%X\n", k->vkCode);
		return CallNextHookEx(NULL, nCode, wParam, lParam);
	}

	if (sigCount > 0)
	{
		int slot = FindSigSlot(ScanCodeOf(k));

		if (slot >= 0)
		{
			if (sig[slot].modifier)
			{
				// Shift/Ctrl/Alt "sûrs" : jamais retenus, toujours transmis en direct
				// (voir IsSafeModifierScanCode) - zéro latence ajoutée sur la frappe ou
				// le jeu normal. On suit juste leur état pour savoir si la combinaison
				// complète est réunie, et pour clore une combinaison déjà active si l'un
				// d'eux est relâché avant le reste.
				BOOL risingEdge = IsKeyDown(wParam) && ! sig[slot].down;
				sig[slot].down = IsKeyDown(wParam);

				// Un front montant alors qu'aucune salve n'est encore en cours (IDLE) ne
				// peut être qu'un appui volontaire de l'utilisateur, tenu par avance,
				// avant même que la touche Copilot ne soit pressée (voir preHeld dans
				// SigSlot) : mémorisé ici, réévalué à chaque nouvel appui.
				if (risingEdge) sig[slot].preHeld = (burstState == BURST_IDLE);

				if (IsKeyUp(wParam) && burstState == BURST_ACTIVE)
				{
					// Seules les touches "retenues" (non-modificateur) font foi : un
					// modificateur reste transmis en direct pendant toute la combinaison
					// (preHeld ou non, voir TryConfirmBurst), donc son état physique peut
					// refléter un usage tout à fait indépendant de l'utilisateur après
					// que le reste de la salve a été relâché. S'y fier laisserait le
					// combo actif indéfiniment tant que ce modificateur reste tenu.
					BOOL stillActive = FALSE;
					for (int i = 0; i < sigCount; i++) if (! sig[i].modifier && sig[i].down && ! sig[i].passedThrough) { stillActive = TRUE; break; }
					if (! stillActive) EndActiveCombo();
				}

				// Le matériel de certaines touches Copilot ne relâche jamais vraiment sa
				// salve tant qu'elle est tenue : il en réémet le "bas" en boucle (répétition
				// clavier de toute la combinaison, sans relâchement entre deux salves).
				// Une fois ce modificateur neutralisé (voir TryConfirmBurst), ne
				// plus retransmettre ces répétitions à l'OS : ça le réaffirmerait "bas" sans
				// jamais être re-neutralisé, laissant Shift/Ctrl/Alt perçu comme tenu pendant
				// toute la durée de la combinaison (restauré/nettoyé dans EndActiveCombo).
				if (sig[slot].neutralized && IsKeyDown(wParam)) return 1;

				LRESULT result = CallNextHookEx(NULL, nCode, wParam, lParam);

				// La touche distinctive (ou Win) peut déjà être en attente si ce
				// modificateur est le dernier de la salve à arriver : réévalue la
				// complétude maintenant que ce modificateur vient de passer à bas.
				if (risingEdge) TryConfirmBurst();

				return result;
			}

			// Touche retenue (Win et/ou touche distinctive de la salve) : confirmation
			// par fenêtre d'attente, réservée aux touches qui ne s'utilisent
			// normalement jamais seules.
			if (IsKeyDown(wParam))
			{
				if (! sig[slot].down)
				{
					sig[slot].down = TRUE;

					if (burstState == BURST_IDLE)
					{
						burstState = BURST_PENDING;
						g_pendingTimerId = SetTimer(NULL, 0, g_pendingWindowMs, PendingTimeoutProc);
						DEBUG_LOG("Signature slot %d down, pending confirmation\n", slot);
					}

					if (burstState == BURST_PENDING)
					{
						// Réévalue tout de suite : si cette touche retenue est la dernière
						// de la salve à arriver (les modificateurs, déjà transmis en direct,
						// étaient tenus avant elle), la combinaison est déjà complète.
						TryConfirmBurst();
						return 1;
					}
				}

				// répétition auto d'une touche déjà tenue
				return sig[slot].passedThrough ? CallNextHookEx(NULL, nCode, wParam, lParam) : 1;
			}
			else if (IsKeyUp(wParam))
			{
				BOOL wasPassed = sig[slot].passedThrough;

				if (! wasPassed && burstState == BURST_PENDING)
				{
					AbortPendingBurst();
					wasPassed = TRUE;
				}

				sig[slot].down = FALSE;
				sig[slot].passedThrough = FALSE;

				if (wasPassed) return CallNextHookEx(NULL, nCode, wParam, lParam);

				if (burstState == BURST_ACTIVE)
				{
					// Même filtre que dans la branche modificateur ci-dessus : seules les
					// touches retenues comptent pour juger si le combo est encore actif.
					BOOL stillActive = FALSE;
					for (int i = 0; i < sigCount; i++) if (! sig[i].modifier && sig[i].down && ! sig[i].passedThrough) { stillActive = TRUE; break; }

					if (! stillActive) EndActiveCombo();
				}
				return 1;
			}
		}
	}

	// Flèches -> Home/End/PgUp/PgDn tant que la touche Copilot est active (ou
	// vient de l'être, cf. g_arrowGraceMs : sur certains claviers le matériel
	// relâche la salve automatiquement au bout de quelques centaines de ms,
	// voir le commentaire sur ARROW_GRACE_MS_DEFAULT). Désactivable
	// (ArrowsEnabled) pour un utilisateur qui ne veut que le remap Ctrl/Menu.
	if (ArrowsEnabled && k->vkCode >= VK_LEFT && k->vkCode <= VK_DOWN)
	{
		BOOL copilotActiveNow = (burstState == BURST_ACTIVE);
		BOOL viaCopilot = copilotActiveNow ||
			(g_burstReleaseTick != 0 && (GetTickCount() - g_burstReleaseTick) <= g_arrowGraceMs);

		for (int i=0; i < 4; i++)
		{
			if (k->vkCode == arrowMaps[i].vkArrow)
			{
				if (IsKeyDown(wParam) && arrowMaps[i].down) {
					// Auto-répétition matérielle de la flèche déjà substituée : le
					// clavier renvoie des WM_KEYDOWN en rafale tant qu'elle reste
					// enfoncée. Sans ce cas, ces répétitions ne correspondaient à
					// aucune branche ci-dessous et retombaient telles quelles jusqu'au
					// CallNextHookEx final : la flèche brute fuitait vers l'OS en plus
					// du Home/End/PgUp/PgDn déjà émis une seule fois.
					if (! viaCopilot) {
						// burstState/ARROW_GRACE_MS ont expiré pendant que la flèche
						// substituée restait tenue (combo Copilot relâché entre-temps,
						// sans jamais relâcher la flèche) : on arrête la substitution
						// ici plutôt que d'attendre le relâchement réel de la flèche,
						// et cette répétition retombe telle quelle jusqu'au
						// CallNextHookEx final pour rendre la flèche à l'OS dès
						// maintenant.
						SendArrow(i, FALSE);
						break;
					}
					SendArrow(i, TRUE);
					return 1;
				}
				else if (viaCopilot && IsKeyDown(wParam) && ! arrowMaps[i].down) {
					DEBUG_LOG("Copilot + Arrow key pressed: vkCode=0x%X\n", k->vkCode);
					// Relâche la sortie Copilot synthétique AVANT d'injecter la flèche
					// substituée (si elle est encore réellement présentée) : sinon elle
					// se combine avec Ctrl droit au lieu d'un Home/End/PgUp/PgDn propre.
					if (copilotActiveNow) SendCopilotKey(FALSE);
					SendArrow(i, TRUE);
					return 1;
				}
				else if (IsKeyUp(wParam) && arrowMaps[i].down) {
					DEBUG_LOG("Copilot + Arrow key released: vkCode=0x%X\n", k->vkCode);
					SendArrow(i, FALSE);
					// Restaure la sortie Copilot seulement si le combo est encore tenu.
					if (burstState == BURST_ACTIVE) SendCopilotKey(TRUE);
					return 1;
				}
			}
		}
	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

// ---------------------------------------------------------------------------
// Mode -config : capture interactive de la signature Copilot
// ---------------------------------------------------------------------------

typedef struct { WORD code; BOOL down; DWORD firstDownTick; } CapKey;

static CapKey g_capBuf[MAX_SIG];
static int    g_capCount;
static DWORD  g_capLastEventTick;

static int CapFind(WORD code)
{
	for (int i = 0; i < g_capCount; i++) if (g_capBuf[i].code == code) return i;
	return -1;
}

static LRESULT CALLBACK CaptureHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode != HC_ACTION) return CallNextHookEx(NULL, nCode, wParam, lParam);

	KBDLLHOOKSTRUCT* k = (KBDLLHOOKSTRUCT*)lParam;
	if (k->flags & LLKHF_INJECTED) return CallNextHookEx(NULL, nCode, wParam, lParam);

	WORD code = ScanCodeOf(k);
	g_capLastEventTick = GetTickCount();

	if (IsKeyDown(wParam))
	{
		int idx = CapFind(code);
		if (idx < 0 && g_capCount < MAX_SIG)
		{
			g_capBuf[g_capCount].code = code;
			g_capBuf[g_capCount].down = TRUE;
			g_capBuf[g_capCount].firstDownTick = g_capLastEventTick;
			g_capCount++;
		}
		else if (idx >= 0) g_capBuf[idx].down = TRUE;
	}
	else if (IsKeyUp(wParam))
	{
		int idx = CapFind(code);
		if (idx >= 0) g_capBuf[idx].down = FALSE;
	}

	return 1; // avale tout pendant la capture
}

// Capture un appui complet (attend qu'au moins une touche soit vue puis que
// tout soit relâché et silencieux pendant CAPTURE_QUIET_MS).
// outSpreadMs : écart entre la 1re et la dernière touche de la salve détectée.
static BOOL CaptureOneAttempt(CapKey* outBuf, int* outCount, DWORD* outSpreadMs)
{
	g_capCount = 0;
	memset(g_capBuf, 0, sizeof(g_capBuf));

	HHOOK capHook = SetWindowsHookEx(WH_KEYBOARD_LL, CaptureHookProc, NULL, 0);
	if (! capHook) return FALSE;

	DWORD startTick = GetTickCount();
	g_capLastEventTick = startTick;

	for (;;)
	{
		MSG msg;
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			Sleep(10);
		}

		DWORD now = GetTickCount();

		BOOL anyDown = FALSE;
		for (int i = 0; i < g_capCount; i++) if (g_capBuf[i].down) { anyDown = TRUE; break; }

		if (g_capCount > 0 && ! anyDown && (now - g_capLastEventTick) > CAPTURE_QUIET_MS) break;
		if ((now - startTick) > CAPTURE_TIMEOUT_MS) break;
	}

	UnhookWindowsHookEx(capHook);

	memcpy(outBuf, g_capBuf, sizeof(g_capBuf));
	*outCount = g_capCount;

	DWORD minTick = 0, maxTick = 0;
	for (int i = 0; i < g_capCount; i++)
	{
		if (i == 0 || g_capBuf[i].firstDownTick < minTick) minTick = g_capBuf[i].firstDownTick;
		if (i == 0 || g_capBuf[i].firstDownTick > maxTick) maxTick = g_capBuf[i].firstDownTick;
	}
	*outSpreadMs = maxTick - minTick;

	return g_capCount > 0;
}

// Calibration de la fenêtre de grâce flèches (voir ARROW_GRACE_MS_DEFAULT) :
// demande à l'utilisateur de tenir la combinaison confirmée (sig[]/sigCount)
// pendant ARROW_GRACE_CALIB_HOLD_MS, puis regarde si le "relâchement" (tous
// les slots de la signature repassent à l'état haut) survient bien avant la
// fin de la consigne. Si oui, c'est le signe d'un auto-release matériel réel,
// indépendant de la tenue physique : on retient la durée mesurée + marge. Si
// le relâchement n'arrive qu'après (proche de la consigne, l'utilisateur a
// bien tenu jusqu'au bout), le matériel suit fidèlement l'état physique et
// aucune fenêtre de grâce n'est nécessaire : on retient le minimum de sécurité.
// Repose sur la même ambiguïté que g_burstReleaseTick en usage normal (aucun
// moyen de distinguer un relâchement matériel prématuré d'un relâchement
// volontaire) - la consigne de durée sert justement à lever cette ambiguïté
// une bonne fois pour toutes, en calibration, plutôt qu'à chaque combo réel.
static DWORD g_calibDownTick;
static BOOL  g_calibAllDown;
static DWORD g_calibReleaseTick;
static BOOL  g_calibReleased;

static LRESULT CALLBACK CalibrateGraceHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode != HC_ACTION) return CallNextHookEx(NULL, nCode, wParam, lParam);

	KBDLLHOOKSTRUCT* k = (KBDLLHOOKSTRUCT*)lParam;
	if (k->flags & LLKHF_INJECTED) return CallNextHookEx(NULL, nCode, wParam, lParam);

	int slot = FindSigSlot(ScanCodeOf(k));
	if (slot >= 0)
	{
		if (IsKeyDown(wParam)) sig[slot].down = TRUE;
		else if (IsKeyUp(wParam)) sig[slot].down = FALSE;

		BOOL allDown = TRUE;
		for (int i = 0; i < sigCount; i++) if (! sig[i].down) { allDown = FALSE; break; }

		if (allDown && ! g_calibAllDown)
		{
			g_calibAllDown = TRUE;
			g_calibDownTick = GetTickCount();
		}
		else if (! allDown && g_calibAllDown && ! g_calibReleased)
		{
			g_calibReleased = TRUE;
			g_calibReleaseTick = GetTickCount();
		}
	}

	return 1; // avale tout pendant la calibration
}

// outGraceMs : fenêtre de grâce retenue. Retourne FALSE si rien n'a pu être
// mesuré (touche jamais complètement enfoncée dans le temps imparti) - dans ce
// cas l'appelant garde ARROW_GRACE_MS_DEFAULT.
static BOOL CalibrateArrowGrace(DWORD* outGraceMs)
{
	for (int i = 0; i < sigCount; i++) sig[i].down = FALSE;
	g_calibAllDown = FALSE;
	g_calibReleased = FALSE;
	g_calibDownTick = 0;
	g_calibReleaseTick = 0;

	HHOOK calibHook = SetWindowsHookEx(WH_KEYBOARD_LL, CalibrateGraceHookProc, NULL, 0);
	if (! calibHook) return FALSE;

	DWORD startTick = GetTickCount();

	for (;;)
	{
		MSG msg;
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			Sleep(10);
		}

		DWORD now = GetTickCount();

		if (g_calibReleased) break;
		if ((now - startTick) > CAPTURE_TIMEOUT_MS + ARROW_GRACE_CALIB_HOLD_MS) break;
	}

	Say("Relâchez la touche [Copilot] maintenant !\n\n", "Release the [Copilot] key now !\n\n");

	Sleep(3000); // laisse le temps à l'utilisateur de relâcher la touche avant de couper le hook

	UnhookWindowsHookEx(calibHook);

	if (! g_calibAllDown || ! g_calibReleased) return FALSE;

	DWORD heldMs = g_calibReleaseTick - g_calibDownTick;

	if (heldMs < ARROW_GRACE_CALIB_THRESHOLD_MS)
	{
		// Relâché bien avant la consigne : auto-release matériel probable.
		*outGraceMs = heldMs + ARROW_GRACE_CALIB_MARGIN_MS;
	}
	else
	{
		// Tenu jusqu'au bout (ou presque) : pas de quirk, minimum de sécurité.
		*outGraceMs = ARROW_GRACE_MS_MIN;
	}

	if (*outGraceMs < ARROW_GRACE_MS_MIN) *outGraceMs = ARROW_GRACE_MS_MIN;
	if (*outGraceMs > ARROW_GRACE_MS_MAX) *outGraceMs = ARROW_GRACE_MS_MAX;

	return TRUE;
}

#define CAPTURE_HID_WINDOW_CLASS "CopilotKeyPlusCapHid"

static WORD g_capHidUsage;
static BOOL g_capHidSeen;

static LRESULT CALLBACK CaptureHidWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_INPUT)
	{
		WORD usage = ParseConsumerUsageFromRawInput((HRAWINPUT)lParam);
		if (usage != 0 && ! g_capHidSeen)
		{
			g_capHidUsage = usage;
			g_capHidSeen = TRUE;
		}
		return 0;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

// Repli déclenché uniquement quand CaptureOneAttempt n'a rien vu (voir
// commentaire en tête de fichier) : écoute en Raw Input les usages de la page
// HID Consumer, pour la touche Copilot matérielle dédiée qui n'émet aucun
// scancode. Fenêtre et enregistrement du périphérique créés et détruits
// localement à chaque appel, pour ne rien coûter aux claviers déjà couverts
// par le mécanisme scancode classique.
static BOOL CaptureOneHidAttempt(WORD* outUsage)
{
	static BOOL classRegistered = FALSE;
	if (! classRegistered)
	{
		WNDCLASSA wc = {0};
		wc.lpfnWndProc = CaptureHidWndProc;
		wc.hInstance = GetModuleHandleA(NULL);
		wc.lpszClassName = CAPTURE_HID_WINDOW_CLASS;
		if (! RegisterClassA(&wc)) return FALSE;
		classRegistered = TRUE;
	}

	HWND hwnd = CreateWindowExA(0, CAPTURE_HID_WINDOW_CLASS, "", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, GetModuleHandleA(NULL), NULL);
	if (! hwnd) return FALSE;

	RAWINPUTDEVICE rid = {0};
	rid.usUsagePage = HID_USAGE_PAGE_CONSUMER;
	rid.usUsage = HID_USAGE_CONSUMERCTRL;
	rid.dwFlags = RIDEV_INPUTSINK;
	rid.hwndTarget = hwnd;

	BOOL result = FALSE;

	if (RegisterRawInputDevices(&rid, 1, sizeof(rid)))
	{
		g_capHidSeen = FALSE;
		g_capHidUsage = 0;

		DWORD startTick = GetTickCount();
		for (;;)
		{
			MSG msg;
			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				Sleep(10);
			}

			if (g_capHidSeen) break;
			if ((GetTickCount() - startTick) > CAPTURE_TIMEOUT_MS) break;
		}

		// Désenregistre le périphérique (RIDEV_REMOVE) avant de détruire la fenêtre.
		rid.dwFlags = RIDEV_REMOVE;
		rid.hwndTarget = NULL;
		RegisterRawInputDevices(&rid, 1, sizeof(rid));

		if (g_capHidSeen)
		{
			*outUsage = g_capHidUsage;
			result = TRUE;
		}
	}

	DestroyWindow(hwnd);
	return result;
}

static BOOL CaptureSetsEqual(const CapKey* a, int aCount, const CapKey* b, int bCount)
{
	if (aCount != bCount) return FALSE;

	for (int i = 0; i < aCount; i++)
	{
		BOOL found = FALSE;
		for (int j = 0; j < bCount; j++) if (a[i].code == b[j].code) { found = TRUE; break; }
		if (! found) return FALSE;
	}
	return TRUE;
}

// GetKeyNameText ne connaît pas toujours F13-F24 (touches "étendues" absentes
// de la plupart des layouts clavier) : repli manuel sur cette plage connue.
static const char* WellKnownScancodeName(WORD code)
{
	static const char* fkeys[] = {
		"F13", "F14", "F15", "F16", "F17", "F18", "F19", "F20", "F21", "F22", "F23"
	};
	if (code >= 0x64 && code <= 0x6E) return fkeys[code - 0x64];
	if (code == K_F24) return "F24";
	return NULL;
}

static void AppendKeyName(char* out, size_t outSize, WORD code)
{
	const char* known = WellKnownScancodeName(code);
	if (known)
	{
		strcat_s(out, outSize, known);
		return;
	}

	LONG lp = ((LONG)(code & 0xFF)) << 16;
	if (code & 0xFF00) lp |= (1L << 24);

	char name[64];
	if (GetKeyNameTextA(lp, name, sizeof(name)) > 0)
		strcat_s(out, outSize, name);
	else
	{
		char hex[16];
		sprintf_s(hex, sizeof(hex), "0x%04X", code);
		strcat_s(out, outSize, hex);
	}
}

static void PrintCombo(const CapKey* buf, int count)
{
	char line[256] = "";
	for (int i = 0; i < count; i++)
	{
		if (i > 0) strcat_s(line, sizeof(line), " + ");
		AppendKeyName(line, sizeof(line), buf[i].code);
	}
	printf("%s%s%s\n", FC_GREEN, line, FC_END);
}

// Affiché en FC_RED (et non FC_GREEN comme PrintCombo) : ce chemin est
// nouveau/expérimental (touche Copilot matérielle dédiée, voir commentaire en
// tête de fichier), il doit visuellement sauter aux yeux pendant l'install.
static void PrintHidUsage(WORD usage)
{
	const char* known = (usage == 0x00D8) ? "Copilot" : NULL;
	if (known)
		printf("%sHID Consumer 0x%04X (%s)%s\n", FC_RED, usage, known, FC_END);
	else
		printf("%sHID Consumer 0x%04X%s\n", FC_RED, usage, FC_END);
}

static void EnableAnsiColors(void)
{
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD mode = 0;
	if (GetConsoleMode(hOut, &mode))
	{
		SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
	}
}

// Taille de police de la console -config : plus lisible que la taille par défaut
// (souvent petite sur les hauts DPI). dwFontSize.X à 0 laisse Windows calculer
// la largeur proportionnellement à la hauteur demandée.
static void SetConsoleFontSize(SHORT sizeY)
{
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_FONT_INFOEX cfi = {0};
	cfi.cbSize = sizeof(cfi);
	if (GetCurrentConsoleFontEx(hOut, FALSE, &cfi))
	{
		cfi.dwFontSize.X = 0;
		cfi.dwFontSize.Y = sizeY;
		SetCurrentConsoleFontEx(hOut, FALSE, &cfi);
	}
}

// Le code source (et donc les littéraux de chaînes) est compilé en UTF-8
// (/utf-8) : la console doit être basculée sur le même codepage pour afficher
// correctement les caractères accentués, sinon ils sont perdus/rendus faux.
static void SetConsoleUtf8(void)
{
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);
}

static int RunConfig(void)
{
	AllocConsole();
	FILE* fpOut; freopen_s(&fpOut, "CONOUT$", "w", stdout);
	FILE* fpIn;  freopen_s(&fpIn, "CONIN$", "r", stdin);
	SetConsoleUtf8();
	EnableAnsiColors();
	SetConsoleFontSize(18);

	printf("\n%sLanguage / Langue%s\n", FC_YELLOW, FC_END);
	printf("  1) English (default)\n");
	printf("  2) Français\n-> ");

	char line[16];
	if (fgets(line, sizeof(line), stdin) && line[0] == '2') g_lang = LANG_FR;

	Say("\n%s===  Installation de Copilot Key+  ===%s\n\n", "\n%s===  Copilot Key+ Setup  ===%s\n\n", FC_YELLOW, FC_END);

	CapKey prevBuf[MAX_SIG] = {0};
	int    prevCount = 0;
	DWORD  prevSpread = 0;
	DWORD  maxSpread = 0;
	BOOL   confirmed = FALSE;
	BOOL   havePrev = FALSE;
	BOOL   prevIsHid = FALSE;
	WORD   prevHidUsage = 0;
	BOOL   confirmedIsHid = FALSE;
	WORD   confirmedHidUsage = 0;

	for (int attempt = 1; attempt <= 4 && ! confirmed; attempt++)
	{
		Say("%sAppuyez sur la touche Copilot (essai %d)...%s\n", "%sPress the Copilot key (attempt %d)...%s\n", FC_YELLOW, attempt, FC_END);

		CapKey buf[MAX_SIG];
		int count = 0;
		DWORD spreadMs = 0;
		BOOL isHid = FALSE;
		WORD hidUsage = 0;

		if (! CaptureOneAttempt(buf, &count, &spreadMs))
		{
			// Repli HID Consumer (touche Copilot matérielle dédiée, voir commentaire
			// en tête de fichier) : uniquement tenté quand la capture scancode
			// classique n'a rien vu du tout sur cet essai.
			if (CaptureOneHidAttempt(&hidUsage))
			{
				isHid = TRUE;
				count = 0;
			}
			else
			{
				Say("%sAucune touche détectée, réessayez.%s\n\n", "%sNo key detected, try again.%s\n\n", FC_YELLOW, FC_END);
				attempt--;
				continue;
			}
		}

		Say("Détecté : ", "Detected : ");
		if (isHid) PrintHidUsage(hidUsage); else PrintCombo(buf, count);
		if (! isHid) DEBUG_LOG("Capture spread: %lu ms\n", spreadMs);

		// Deux essais ne se confirment mutuellement que s'ils sont du même type
		// (tous les deux scancode, ou tous les deux HID avec le même usage) : un
		// essai scancode suivi d'un essai HID (ou l'inverse) est un mismatch, au
		// même titre que deux salves scancode différentes.
		BOOL matches = havePrev && (isHid == prevIsHid) &&
			(isHid ? (hidUsage == prevHidUsage) : CaptureSetsEqual(buf, count, prevBuf, prevCount));

		if (matches)
		{
			confirmed = TRUE;
			confirmedIsHid = isHid;
			confirmedHidUsage = hidUsage;
			if (! isHid)
			{
				if (spreadMs > maxSpread) maxSpread = spreadMs;
				if (prevSpread > maxSpread) maxSpread = prevSpread;
			}
		}
		else
		{
			if (havePrev) Say("%sLes deux essais ne correspondent pas, on réessaie.%s\n", "%sThe two attempts don't match, retrying.%s\n", FC_YELLOW, FC_END);
			else Say("Appuyez à nouveau sur la touche Copilot pour confirmer.\n", "Press the Copilot key again to confirm.\n");
		}

		memcpy(prevBuf, buf, sizeof(buf));
		prevCount = count;
		prevSpread = spreadMs;
		prevIsHid = isHid;
		prevHidUsage = hidUsage;
		havePrev = TRUE;
		printf("\n");
	}

	if (! confirmed)
	{
		Say("%sÉchec de la capture après plusieurs essais. Installation annulée !%s\n", "%sCapture failed after several attempts. Setup cancelled !%s\n", FC_RED, FC_END);
		Say("Appuyez sur [Entrée] pour fermer...", "Press [Enter] to close...");
		(void)getchar();
		FreeConsole();
		return 1;
	}

	g_sigIsHid = confirmedIsHid;
	g_hidUsage = confirmedHidUsage;
	sigCount = 0;
	if (! confirmedIsHid)
	{
		for (int i = 0; i < prevCount && i < MAX_SIG; i++)
		{
			sig[sigCount].code = prevBuf[i].code;
			sig[sigCount].down = FALSE;
			sig[sigCount].passedThrough = FALSE;
			sig[sigCount].modifier = IsSafeModifierScanCode(sig[sigCount].code);
			sig[sigCount].neutralized = FALSE;
			sig[sigCount].preHeld = FALSE;
			sigCount++;
		}
	}

	Say("Combinaison retenue : ", "Combination kept : ");
	if (confirmedIsHid) PrintHidUsage(confirmedHidUsage); else PrintCombo(prevBuf, prevCount);
	printf("\n");

	// Vérifie que l'hypothèse dont dépend KeyboardProc tient sur ce clavier :
	// au moins une touche "distinctive" (ni Shift/Ctrl/Alt), et les modificateurs
	// "sûrs" enfoncés avant elle dans la salve (sinon la neutralisation ponctuelle
	// à la confirmation arriverait trop tard pour éviter de polluer la touche
	// Copilot synthétique). Sans objet en mode HID Consumer (pas de salve).
	if (! confirmedIsHid)
	{
		BOOL haveGate = FALSE;
		DWORD gateFirstDown = 0;
		for (int i = 0; i < prevCount; i++) if (! IsSafeModifierScanCode(prevBuf[i].code)) { haveGate = TRUE; gateFirstDown = prevBuf[i].firstDownTick; break; }

		if (! haveGate)
		{
			Say("%sAttention : combinaison composée uniquement de touches Shift/Ctrl/Alt, aucune touche distinctive détectée. La détection utilisera l'ancien mécanisme (délai possible).%s\n\n",
			    "%sWarning: combination made only of Shift/Ctrl/Alt keys, no distinctive key detected. Detection will fall back to the old mechanism (possible delay).%s\n\n", FC_RED, FC_END);
		}
		else
		{
			for (int i = 0; i < prevCount; i++)
			{
				if (IsSafeModifierScanCode(prevBuf[i].code) && prevBuf[i].firstDownTick > gateFirstDown)
				{
					Say("%sAttention : un modificateur (Shift/Ctrl/Alt) arrive après la touche distinctive dans la salve. La touche Copilot synthétique pourrait occasionnellement apparaître combinée avec lui.%s\n\n",
					    "%sWarning: a modifier (Shift/Ctrl/Alt) arrives after the distinctive key in the burst. The synthetic Copilot key might occasionally show up combined with it.%s\n\n", FC_DARKGRAY, FC_END);
					break;
				}
			}
		}
	}

	char sigStr[256] = "";
	char hidStr[16] = "";
	DWORD windowValue = PENDING_WINDOW_MS_DEFAULT;
	DWORD graceValue = 0;
	BOOL  haveGraceValue = FALSE;

	if (confirmedIsHid)
	{
		sprintf_s(hidStr, sizeof(hidStr), "%04X", confirmedHidUsage);
	}
	else
	{
		SerializeSignature(sigStr, sizeof(sigStr));

		// Fenêtre de confirmation = écart mesuré x2 (marge de sécurité), bornée.
		windowValue = maxSpread * 2;
		if (windowValue < PENDING_WINDOW_MS_MIN) windowValue = PENDING_WINDOW_MS_MIN;
		if (windowValue > PENDING_WINDOW_MS_MAX) windowValue = PENDING_WINDOW_MS_MAX;

		Say("Écart mesuré entre les touches de la salve : %lu ms (fenêtre retenue : %lu ms)\n\n",
		    "Measured spread between the burst's keys : %lu ms (window kept: %lu ms)\n\n", maxSpread, windowValue);

		// Calibrée juste après la détection, pendant que la combinaison est encore
		// "en main" - indépendamment du choix [Flèches] à venir (voir plus bas) :
		// la valeur n'est écrite en registre que si l'utilisateur active la
		// fonctionnalité, mais la mesurer maintenant évite d'interrompre le geste
		// de l'utilisateur avec des questions de menu entre les deux.
		Say("%sCalibration de la fenêtre de grâce%s\n\n", "%sCalibrating the grace window%s\n\n", FC_YELLOW, FC_END);
		Say("  - Appuyez sur la touche [Copilot] sans la relâcher\n", "  - Press the [Copilot] key without releasing it\n");
		Say("  - Appuyez 2 ou 3 fois sur une touche quelconque (de [A] à [Z]) pendant ce temps\n", "  - Press 2 or 3 times any key (from [A] to [Z]) while holding it\n");
		Say("  - Puis relâchez [Copilot] normalement...\n\n", "  - Then release [Copilot] normally...\n\n");

		Say("\nAppuyez sur [Entrée] dès que vous êtes prêt...\n", "\nPress [Enter] when you are ready...\n");
		(void)getchar();

		Say("%sPartez !%s\n\n", "%sGo !%s\n\n", FC_RED, FC_END);
		
		haveGraceValue = CalibrateArrowGrace(&graceValue);

		if (haveGraceValue)
		{
			Say("Tenue mesurée : %lu ms (fenêtre de grâce retenue : %lu ms)\n\n",
			    "Measured hold: %lu ms (grace window kept: %lu ms)\n\n", g_calibReleaseTick - g_calibDownTick, graceValue);
		}
		else
		{
			Say("%sCalibration non concluante, valeur par défaut conservée (%d ms).%s\n\n",
			    "%sCalibration inconclusive, default value kept (%d ms).%s\n\n", FC_YELLOW, ARROW_GRACE_MS_DEFAULT, FC_END);
		}
	}

	Say("%sQue doit produire la touche [Copilot] ?%s\n", "%sWhat should the [Copilot] key produce ?%s\n", FC_YELLOW, FC_END);
	Say("  1) CTRL droit -> par défaut, si vous appuyez sur [Entrée]\n", "  1) Right CTRL -> default, if you press [Enter]\n");
	Say("  2) Touche Menu contextuel\n", "  2) Context menu key\n");
	Say("  3) Rien (désactivée)\n-> ", "  3) Nothing (disabled)\n-> ");

	int modeChoice = 1;
	if (fgets(line, sizeof(line), stdin)) { int v = atoi(line); if (v >= 1 && v <= 3) modeChoice = v; }

	DWORD modeValue;
	switch (modeChoice) {
		case 2:  modeValue = 2; break;
		case 3:  modeValue = 0; break;
		default: modeValue = 1; break;
	}

	Say("\n%sVoulez-vous activer les combinaisons [Copilot] + [Flèches] ?%s\n", "\n%sDo you want to enable the [Copilot] + [Arrow Keys] combinations ?%s\n", FC_YELLOW, FC_END);
	Say("(O/N) -> \"Oui\" par défaut si vous appuyez sur [Entrée]\n-> ", "(Y/N) -> \"Yes\" by default if you press [Enter]\n-> ");

	DWORD arrowsValue = 1;
	if (fgets(line, sizeof(line), stdin))
	{
		if (line[0] == 'n' || line[0] == 'N') arrowsValue = 0;
	}

	HKEY hKey;
	if (RegCreateKeyExA(HKEY_CURRENT_USER, REG_KEY, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS)
	{
		if (confirmedIsHid)
		{
			RegSetValueExA(hKey, "HIDconsumer", 0, REG_SZ, (const BYTE*)hidStr, (DWORD)strlen(hidStr) + 1);
			// Nettoyage d'un résidu éventuel d'une installation scancode précédente
			// sur ce même compte (réinstall après changement de clavier) : Signature
			// est inerte en mode HID (jamais relue), mais Grace ne l'est pas - un
			// résidu calibré pour l'ancien clavier scancode continuerait sinon à
			// s'appliquer au chemin HID au lieu de retomber sur ARROW_GRACE_MS_DEFAULT.
			RegDeleteValueA(hKey, "Signature");
			RegDeleteValueA(hKey, "Burst");
			RegDeleteValueA(hKey, "Grace");
		}
		else
		{
			RegSetValueExA(hKey, "Signature", 0, REG_SZ, (const BYTE*)sigStr, (DWORD)strlen(sigStr) + 1);
			// Nettoyage d'un résidu éventuel d'une installation HID précédente sur
			// ce même compte (réinstall après changement de clavier).
			RegDeleteValueA(hKey, "HIDconsumer");
			RegSetValueExA(hKey, "Burst", 0, REG_DWORD, (const BYTE*)&windowValue, sizeof(DWORD));
			if (haveGraceValue && arrowsValue) RegSetValueExA(hKey, "Grace", 0, REG_DWORD, (const BYTE*)&graceValue, sizeof(DWORD));
			else RegDeleteValueA(hKey, "Grace"); // retombe sur ARROW_GRACE_MS_DEFAULT au chargement
		}
		RegSetValueExA(hKey, "Mode", 0, REG_DWORD, (const BYTE*)&modeValue, sizeof(DWORD));
		RegSetValueExA(hKey, "Arrows", 0, REG_DWORD, (const BYTE*)&arrowsValue, sizeof(DWORD));
		RegCloseKey(hKey);
	}

	// Le démarrage automatique (clé Run / raccourci du dossier Démarrage) est du
	// ressort de l'installateur, pas de cet exécutable : voir Resources/Install.nsi.
	// Ici, on ne fait qu'appliquer les réglages qui viennent d'être capturés à
	// l'instance résidente déjà en cours, s'il y en a une.

	// Arrête proprement une éventuelle instance résidente déjà en cours (ex :
	// reconfiguration via "-config" alors que Copilot Key+ tourne déjà), et
	// attend sa sortie effective : le mutex "Global\CopilotKeyPlus" n'est
	// relâché qu'à la fin de son WinMain, la relance ci-dessous échouerait
	// silencieusement (nouvelle instance qui se termine aussitôt) sans cette
	// attente.
	HWND running = FindWindowExA(HWND_MESSAGE, NULL, QUIT_WINDOW_CLASS, NULL);
	if (running)
	{
		DWORD pid = 0;
		GetWindowThreadProcessId(running, &pid);
		PostMessageA(running, WM_CLOSE, 0, 0);

		if (pid)
		{
			HANDLE hProc = OpenProcess(SYNCHRONIZE, FALSE, pid);
			if (hProc)
			{
				WaitForSingleObject(hProc, 3000);
				CloseHandle(hProc);
			}
		}
	}

	char exePath[MAX_PATH];
	GetModuleFileNameA(NULL, exePath, MAX_PATH);

	// Démarre (ou redémarre) l'instance résidente tout de suite, sans attendre
	// la prochaine ouverture de session. DETACHED_PROCESS + bInheritHandles=FALSE :
	// le nouveau processus n'hérite d'aucun handle de la console d'installation,
	// il lui survit donc sans encombre une fois celle-ci fermée.
	STARTUPINFOA si = {0};
	si.cb = sizeof(si);
	PROCESS_INFORMATION pi = {0};
	BOOL started = CreateProcessA(exePath, NULL, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &si, &pi);
	if (started)
	{
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}

	if (started)
	{
		Say("\n%sConfiguration terminée. Copilot Key+ est démarré.%s\n",
		    "\n%sSetup complete. Copilot Key+ is now running.%s\n", FC_GREEN, FC_END);
	}
	else
	{
		Say("\n%sConfiguration terminée, mais le redémarrage automatique a échoué : relancez Copilot Key+ manuellement.%s\n",
		    "\n%sSetup complete, but the automatic restart failed: please relaunch Copilot Key+ manually.%s\n", FC_YELLOW, FC_END);
	}
	Say("\nAppuyez sur [Entrée] pour fermer...\n\n", "\nPress [Enter] to close...\n\n");
	(void)getchar();

	FreeConsole();
	return 0;
}

// Fenêtre cachée (message-only) de l'instance résidente : reçoit WM_CLOSE
// depuis "CopilotKey+.exe -quit" et déclenche l'arrêt propre habituel (WM_QUIT).
// Reçoit aussi WM_INPUT quand g_sigIsHid (touche Copilot matérielle dédiée,
// voir commentaire en tête de fichier) : Raw Input n'est enregistré que dans
// ce cas (voir WinMain), donc aucun coût pour les autres utilisateurs.
static LRESULT CALLBACK HiddenWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_CLOSE)
	{
		PostQuitMessage(0);
		return 0;
	}

	if (msg == WM_INPUT && g_sigIsHid)
	{
		WORD usage = ParseConsumerUsageFromRawInput((HRAWINPUT)lParam);
		BOOL down = (usage == g_hidUsage);

		if (down && ! g_hidKeyDown)
		{
			g_hidKeyDown = TRUE;
			DEBUG_LOG("HID Consumer Copilot key down (usage 0x%04X)\n", g_hidUsage);
			// Même effet que TryConfirmBurst() pour le chemin scancode : fait
			// fonctionner gratuitement le remap Copilot+Flèches (KeyboardProc ne
			// regarde que burstState).
			burstState = BURST_ACTIVE;
			SendCopilotKey(TRUE);
		}
		else if (! down && g_hidKeyDown)
		{
			g_hidKeyDown = FALSE;
			DEBUG_LOG("HID Consumer Copilot key up\n");
			EndActiveCombo();
		}
		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

// Pas d'AttachConsole(ATTACH_PARENT_PROCESS) ici : sur certains hôtes (Windows
// Terminal, terminal intégré VS Code, ConPTY en général), cet appel peut se
// bloquer durablement, ce qui empêche la console appelante de reprendre la
// main. Le message de confirmation n'est pas essentiel - le code de sortie
// (0 = arrêté, 1 = aucune instance) suffit - donc on s'en passe pour garantir
// un retour immédiat.
static int RunQuit(void)
{
	HWND target = FindWindowExA(HWND_MESSAGE, NULL, QUIT_WINDOW_CLASS, NULL);
	if (target)
	{
		PostMessage(target, WM_CLOSE, 0, 0);
		return 0;
	}
	return 1;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(nCmdShow);

	if (lpCmdLine && strstr(lpCmdLine, "-config") != NULL)
	{
		return RunConfig();
	}

	if (lpCmdLine && strstr(lpCmdLine, "-quit") != NULL)
	{
		return RunQuit();
	}

	#ifdef LOG
		AllocConsole();
		FILE* fp;
		freopen_s(&fp, "CONOUT$", "w", stdout);
		DEBUG_LOG("Debug console initialized\n");
	#endif

	// Mutex pour instance unique (portée session Windows globale)
	HANDLE hMutex = CreateMutexA(NULL, FALSE, "Global\\CopilotKeyPlus");

	if (GetLastError() == ERROR_ALREADY_EXISTS) return 0;

	// Priorité haute pour limiter la gigue du hook clavier bas niveau sous forte
	// charge CPU (jeu, etc.), sans le risque pour la stabilité système que ferait
	// REALTIME_PRIORITY_CLASS sur un process avec un hook clavier global permanent.
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

	WNDCLASSA wc = {0};
	wc.lpfnWndProc = HiddenWndProc;
	wc.hInstance = hInst;
	wc.lpszClassName = QUIT_WINDOW_CLASS;
	RegisterClassA(&wc);
	HWND hiddenWnd = CreateWindowExA(0, QUIT_WINDOW_CLASS, "Copilot Key+", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInst, NULL);

	LoadRegistrySettings();

	// Écoute Raw Input uniquement pour les utilisateurs dont la touche Copilot
	// matérielle dédiée a été apprise à l'install (g_sigIsHid) : zéro overhead
	// pour l'immense majorité, qui reste sur le mécanisme scancode classique.
	if (g_sigIsHid)
	{
		RAWINPUTDEVICE rid = {0};
		rid.usUsagePage = HID_USAGE_PAGE_CONSUMER;
		rid.usUsage = HID_USAGE_CONSUMERCTRL;
		rid.dwFlags = RIDEV_INPUTSINK;
		rid.hwndTarget = hiddenWnd;
		RegisterRawInputDevices(&rid, 1, sizeof(rid));
	}

	HHOOK hook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, hInst, 0);
	if (! hook) return 1;

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (msg.message == WM_QUIT || (msg.message == WM_ENDSESSION && msg.wParam == TRUE))
		{
			if (copilotDown) SendCopilotKey(FALSE);
			break;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	UnhookWindowsHookEx(hook);
	ReleaseMutex(hMutex);
	CloseHandle(hMutex);

	#ifdef LOG
		DEBUG_LOG("Exiting application\n");
		if (fp) fclose(fp);
		FreeConsole();
	#endif

	return 0;
}
