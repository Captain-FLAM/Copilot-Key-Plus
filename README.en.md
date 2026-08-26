# <sub>![](images/Copilot_48x48.png)</sub> Copilot Key+

#### 🇫🇷 [Version en français](README.md)

**Copilot Key+** intercepts the `Copilot` key and gives it back the behavior of a right **`CTRL`** key (or the `Context menu` key, or no action at all - your choice).

As a bonus, it restores `Home` / `End` / `Page Up` / `Page Down` on laptops, freeing up access to the numeric keypad (digits).

### 🛡️ Antivirus false positives on VirusTotal (2 out of 69) ?

> **That's normal**: it intercepts keyboard keystrokes... (🔒)

| Antivirus | False positive | Description |
| --- | --- | --- |
| Microsoft | `Trojan:Win32/Wacatac.B!ml` | Generic machine-learning **heuristic detection**, triggered by the low-level keyboard hook (typical keylogger-like behavior, even when legitimate). |
| Bkav Pro | `W32.Malware.E9A34DE9` | **Generic detection**, same cause: system-level key interception. |

> **Microsoft**, of all people! He doesn't like it when you mess with his system!! Especially to hijack the `Copilot` key! 😏

#### In the meantime, **67 others** (Kaspersky, Norton, McAfee, etc.) consider it **NOT** to be a virus !!

[Virus Total ➡️](https://www.virustotal.com/gui/file/6ceaab5f6fe365a5f1b6785c02ce30e62d2ecaccbc5a26a235ed65e706d75faa)

All the code is here, in this GitHub repository → Compile it yourself: **End of the debate!**

---

## 🍺 A beer for the old programmer ?

This project is free, ad-free, has no telemetry, and will stay that way.
It has cost me months of tinkering and countless nights deciphering bursts of keyboard scancodes, all to give you back **your** `CTRL` key.

If it has made your life a little easier, a small donation **(clicking on the cup)** could also make the person who wrote it feel a little better :  
[![ko-fi](images/ko-fi.png)](https://ko-fi.com/captain_flam)

---

## 📖 My story

I've been coding for a long time.

Long enough for my left thumb to find the **CTRL** key with my eyes closed, at the bottom left of the keyboard, exactly where it has always been!

`Ctrl + C`, `Ctrl + V`, `Ctrl + Z`, `Ctrl + Shift + Arrow`... tens of thousands of times a day, without even thinking about it.  
That's what muscle memory looks like for an old programmer.

Then I got a new laptop. First reflex, bottom left: nothing.  
Or rather - a **Copilot** key, imposed by the manufacturer **under pressure from Microsoft**, planted right where my CTRL key had always lived.  
Nobody asked me, nobody offered it to me: it's just there, imposed, popping up a window I couldn't care less about every time I fumble a keystroke.

As if that weren't enough: this PC has a numeric keypad. A real one, complete, with its neat little aligned keys. Except I'm still missing the **Home / End / Page Up / Page Down** group you'd get on a proper keyboard.

To navigate through my code, I now have to either disable the numeric keypad, or juggle `Left Shift` + `Right Shift` + `3` on the keypad (just to select with `Page Down`)!

An old programmer like me doesn't need these complications...  
So I did what old programmers do: I wrote a program to fix my keyboard from Microsoft's **misdeeds**.

---

## ⚙️ What this program does

**Copilot Key+** is a small Windows utility that installs itself as a resident (invisible) process and intercepts the hardware combination sent by the Copilot key (`Win + C`, `Win + Shift + F23`, `Win + Ctrl + F23`, or others...) before it ever reaches Windows.

During installation, the program **learns** the specific combination for your machine: just press the Copilot key twice. There's no fixed table by brand - each keyboard is measured and recognized individually. Once this signature is captured, all that's left is to choose what the key should actually produce:

- 🅲 **Right `CTRL`** (the default choice, and the most common)
- 📋 the **`Context menu`** key
- 🚫 nothing at all, to neutralize it completely

As a bonus, as long as the Copilot key is held down, the arrow keys turn into **Home / End / Page Up / Page Down**. Many recent laptops pack in a full numeric keypad but, for lack of space, sacrifice the dedicated navigation cluster: reaching it then means juggling the **Fn** or **Shift** key.

**Copilot Key+** replaces that fumbling by driving the keyboard at software level, with no firmware ever needing to be modified: Copilot becomes the **MAGIC key** that was missing.

---

# ⌨️ My Keyboard, My Rules!!

This program isn't going to change the world. But it gave me back something I thought I'd lost: **THE SHEER JOY** of typing on my keyboard without having to fight it. 💖

No more Copilot popup jumping out uninvited the moment thirty years of muscle memory kick in. My numpad is mine again, with `Home`, `End`, `Page Up`, and `Page Down` right at my ring finger, in combination with the `Copilot` key. And my `CTRL` key is finally back where my fingers expect to find it.

These are just small things. But when you've spent years building up muscle memory, it shouldn't be up to a keyboard maker or Microsoft to decide it needs rewriting for you.

So if your keyboard has been fighting you too, ever since someone, somewhere, decided a shortcut to an A.I assistant mattered more than your own reflexes, this little program might be for you too.

**Free, no complicated install, and no need to ask anyone's permission.** 🎉

---

## 📥 Installation

Copilot Key+ is installed with 📥 **[`Copilot Key+ - Install.exe`](https://github.com/Captain-FLAM/Copilot-Key-Plus/releases/latest/download/Copilot%20Key%2B%20-%20Install.exe)** (latest release of the repo): choose your language, accept the license, pick a destination folder (directly inside your user folder, `%UserProfile%`, by default - no admin rights required), then tick (or untick) the box to launch automatically with Windows (`Run` registry key, checked by default).

Once the files are copied, a dedicated screen offers to launch configuration: it walks you through capturing the Copilot signature specific to your machine (see above), choosing what the key should do, and enabling or disabling the arrow-key remap. You can rerun it at any time without reinstalling, via the **Configure Copilot Key+** shortcut added to the Start menu.

To cleanly stop the resident instance, without going through Task Manager, use the **Quit Copilot Key+** shortcut in the Start menu (or `Copilot_Key+.exe -quit` from its install folder).

To uninstall, use the **Uninstall Copilot Key+** shortcut in the Start menu or the dedicated button in *Installed apps* (Windows Settings): both relaunch the installer, which detects the existing installation and offers to **repair** or **uninstall** it instead of starting over. The installer copies itself into the install folder at the end of the process, so there's no need to keep the downloaded file around - there's no separate file for uninstalling.

> ⚠️ **Dedicated hardware Copilot key (untested)**: some very recent keyboards (Windows 11 23H2 and later) have a **dedicated hardware** Copilot key, distinct from the classic key burst (`Win+C`, `Win+Shift+F23`, etc.). This case is handled as an automatic fallback during setup, but I haven't had the chance to test it on a keyboard with this key. If `-config` loops on "No key detected", or if it doesn't behave as expected on this kind of keyboard, please open an issue on the repo.

---

## 🔀 Key combinations

These options are selected during installation and can be changed at any time :

### Copilot (alone)

| Setting chosen | Result |
| --- | --- |
| 1) Right CTRL *(default)* | Copilot ⇒ ⌃ **Right CTRL** |
| 2) Context menu | Copilot ⇒ 📋 **Context menu** |
| 3) Disabled | Copilot ⇒ 🚫 **Nothing** |

The arrow-key remap (Copilot held + arrow) below is an **independent** setting from this choice.  
It completely changes the result of `Copilot + Arrow`.

*(With `Context menu` or `Disabled` instead of `Right CTRL`, these combinations have no particular effect: the arrow behaves normally, as if Copilot didn't exist.)*

### 1) If the arrow-key remap is **enabled**

#### Copilot + Arrows

As long as Copilot is held down:

| Combination | Result |
| --- | --- |
| Copilot + ⬅️ | 🏠 **Home** |
| Copilot + ➡️ | 🏁 **End** |
| Copilot + ⬆️ | ⏫ **Page Up** |
| Copilot + ⬇️ | ⏬ **Page Down** |

#### Copilot MULTI

Copilot only turns the arrow into Home/End/Page Up/Page Down:

if **SHIFT** and/or **CTRL** are held down as well (physically, on the keyboard), they keep being passed straight through to Windows and combine with that result - exactly as they would on a keyboard with a real dedicated navigation cluster.

So the combinations multiply naturally:

| Combination | Result |
| --- | --- |
| SHIFT + Copilot + ⬅️ | 🔤 Select to the start of the line |
| SHIFT + Copilot + ➡️ | 🔤 Select to the end of the line |
| SHIFT + Copilot + ⬆️ | 🔤 Select up one page |
| SHIFT + Copilot + ⬇️ | 🔤 Select down one page |
| CTRL + Copilot + ⬅️ | 📄 Very start of the document |
| CTRL + Copilot + ➡️ | 📄 Very end of the document **(Note \*)** |
| CTRL + SHIFT + Copilot + ⬅️ | 📄 Select to the very start of the document |
| CTRL + SHIFT + Copilot + ➡️ | 📄 Select to the very end of the document |

*(These are standard Windows shortcuts - SHIFT = selection, CTRL = whole document - so the exact result depends on the active application, just like on any keyboard.)*

**Note \*** : This key combination works on my keyboard, but only if you press:  
CTRL (normally) + **[** Copilot ➡️ **]** at lightning speed !!

> This is not a bug: it is caused by the keyboard's hardware controller. And I haven't found a workaround yet...

### 2) Arrow-key remap **disabled**

Copilot then behaves solely as the `Right CTRL` key - the arrow keys are no longer remapped, and Copilot acts as a simple modifier key.

#### Copilot + Arrows

| Combination | Equivalent to | Result |
| --- | --- | --- |
| Copilot + ⬅️ | CTRL + ⬅️ | 🔤 Previous word |
| Copilot + ➡️ | CTRL + ➡️ | 🔤 Next word |
| Copilot + ⬆️ | CTRL + ⬆️ | 🔤 Scrolling up |
| Copilot + ⬇️ | CTRL + ⬇️ | 🔤 Scrolling down |

*(Ctrl+Up/Down depends a bit more on the application than Ctrl+Left/Right: scrolling in most text editors, sometimes no effect elsewhere.)*

#### Copilot MULTI

**SHIFT** and/or **CTRL** held down physically as well keep combining normally, exactly as with `Right CTRL` alone:

| Combination | Equivalent to | Result |
| --- | --- | --- |
| SHIFT + Copilot + ⬅️ | SHIFT + CTRL + ⬅️ | 🔤 Select previous word |
| SHIFT + Copilot + ➡️ | SHIFT + CTRL + ➡️ | 🔤 Select next word |

---

> ⚠️ **Important - press order for `SHIFT + Copilot + Arrow`** *(applies regardless of the setting above)*: on some keyboards, the Copilot key's hardware burst itself includes the `SHIFT` key (example: `Win + Shift + F23`). The program then cannot tell a genuine `SHIFT` press apart from the burst's own hardware noise - it's the same scancode. For `SHIFT + Copilot + Arrow` to work on these keyboards, you must **press `SHIFT` before Copilot**, not the other way around: hold `SHIFT`, *then* press Copilot, *then* the arrow. If the order is reversed, `SHIFT` will be silently ignored.
>
> ⚠️ On some keyboards, a `CTRL + Copilot + Arrow` combination where the `Copilot` key is **held down for a while** before pressing the arrow can occasionally go undetected (hardware collision with the Copilot burst's repeat signal) - press `Copilot` and then **the arrow quickly** right **after** for it to work reliably.

---

## 🕰️ A word on this project

This little program is the result of several **months** of scattered research and testing - figuring out how Windows exposes these keys, measuring the scancode bursts specific to each manufacturer, trying, failing, setting it aside, coming back to it weeks later... so exhausted...

And then one day, I got back into it seriously, with **Claude.ai** in **VS Code** at my side - and everything that had been piling up for months was finally cleaned up, consolidated, and finished in 6 days of very hard work ! 🚀

## ⭐ Did you like it?

If Copilot Key+ gave you back a bit of pleasure typing on your keyboard too, **leave a Star** ⭐ on this repo: it's free, takes two seconds, and helps other tormented keyboards stumble onto it.

And if you know someone whose Copilot key is driving them to despair, let people know about it - a shared link, a mention in passing, or just word of mouth : ***« Spread the word »*** , every victory counts against **Microsoft's misdeeds**. 😄

---

## 📜 License

© Captain FLAM - 2026

This project is released under the **MIT License**. In short: you're free to use, copy, modify, merge, publish, distribute, and even sell copies of it, for free or for profit, as long as you keep the copyright notice and this license notice in all copies or substantial portions of the software.

The software is provided "as is", without warranty of any kind, express or implied, including but not limited to the warranties of merchantability, fitness for a particular purpose, and noninfringement. In no event shall the authors be liable for any claim, damages, or other liability, whether in an action of contract, tort, or otherwise, arising from, out of, or in connection with the software or the use or other dealings in the software.

Icon free to use, commercial use allowed with no attribution required: [uxwing.com/copilot-icon](https://uxwing.com/copilot-icon/)

---
