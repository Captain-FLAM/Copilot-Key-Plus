# <sub>![](images/Copilot_48x48.png)</sub> Copilot Key+

#### 🇬🇧 [Read this in English](README.en.md)

**Copilot Key+** intercepte la touche `Copilot` et lui rend le comportement de la touche **`CTRL` droit** (ou `Menu contextuel`, ou sans action... au choix).

En bonus, il restitue aux PC portables les touches `Début` / `Fin` / `Page Haut` / `Page Bas` et libère ainsi l'accès au pavé numérique (chiffres).

### 🛡️ Faux positifs antivirus sur VirusTotal (2 sur 69) ?

> **C'est normal** : ça intercepte les touches du clavier... (🔒)

| Antivirus | Faux positif | Description |
| --- | --- | --- |
| Microsoft | `Trojan:Win32/Wacatac.B!ml` | **Détection heuristique** par machine learning, déclenchée par le hook clavier bas niveau (comportement typique d'un keylogger, même légitime). |
| Bkav Pro | `W32.Malware.E9A34DE9` | **Détection générique**, même cause : interception des touches au niveau système. |

> **Microsoft**, comme par hasard ! Il n'aime pas qu'on touche à son système !! Surtout pour détourner la touche `Copilot` ! 😏

#### En attendant **67 autres** (Kaspersky, Norton, McAfee, etc...) le considèrent comme n'étant **PAS** un virus !!

[Virus Total ➡️](https://www.virustotal.com/gui/file/6ceaab5f6fe365a5f1b6785c02ce30e62d2ecaccbc5a26a235ed65e706d75faa)

Tout le code est ici, dans ce dépôt GitHub → Compilez-le vous-même : **Fin de polémique !**

---

## 🍺 Une bière pour le vieux programmeur ?

Ce projet est gratuit, sans pub, sans télémétrie, et le restera.  
Il m'a coûté des mois de bidouille et des nuits à décortiquer des salves de scancodes des touches du clavier, pour vous rendre **votre** touche `CTRL`.

Si ça vous a fait du bien, un petit don **(en cliquant sur la tasse)** ferait aussi du bien à celui qui l'a écrit :  
[![ko-fi](images/ko-fi.png)](https://ko-fi.com/captain_flam)

---

## 📖 Mon histoire

Je code depuis longtemps.

Assez longtemps pour que mon pouce gauche sache retrouver la touche **CTRL** les yeux fermés, en bas à gauche du clavier, exactement là où elle a toujours été !

`Ctrl + C`, `Ctrl + V`, `Ctrl + Z`, `Ctrl + Maj + Flèche`... des dizaines de milliers de fois par jour, sans même y penser.  
C'est ça, la mémoire musculaire d'un vieux programmeur.

Et puis j'ai changé d'ordinateur portable. Premier réflexe en bas à gauche : plus rien.  
Ou plutôt si - une touche **Copilot**, imposée par le fabricant **sous la pression de Microsoft**, plantée là où ma touche CTRL vivait depuis toujours.  
Personne ne me l'a demandé, personne ne me l'a proposée : elle est juste là, imposée, à ouvrir une popup dont je n'ai que faire à chaque frappe malheureuse.

Et comme si ça ne suffisait pas : ce PC a un pavé numérique. Un vrai, complet, avec ses jolies petites touches bien alignées. Sauf qu'il me manque toujours le groupe **Début / Fin / Page Haut / Page Bas** comme sur un vrai clavier.

Pour naviguer dans mon code, il faut désormais que je désactive le pavé numérique, ou que je jongle avec `MAJ` gauche + `MAJ` droit + `3` du pavé (pour sélectionner avec `Page Bas`) !

Un vieux programmeur comme moi, n'a pas besoin de ces complications...  
Alors j'ai fait ce que font les vieux programmeurs : j'ai écrit un programme pour réparer mon clavier des **méfaits de Microsoft**.

---

## ⚙️ Ce que fait ce programme

**Copilot Key+** est un petit utilitaire Windows qui s'installe en résident (invisible) et qui intercepte la combinaison matérielle envoyée par la touche Copilot (`Win + C`, `Win + Maj + F23`, `Win + Ctrl + F23`, ou autre...) avant qu'elle n'atteigne Windows.

À l'installation, le programme **apprend** la combinaison spécifique de la machine : il suffit d'appuyer deux fois sur la touche Copilot. Il n'existe pas de table figée par marque, chaque clavier est mesuré et reconnu individuellement. Une fois cette signature capturée, il reste à choisir ce que la touche doit réellement produire :

- 🅲 **`CTRL` droit** (le choix par défaut, et le plus courant)
- 📋 la touche **`Menu contextuel`**
- 🚫 rien du tout, pour la neutraliser complètement

En bonus, tant que la touche Copilot est maintenue enfoncée, les flèches directionnelles du clavier se transforment en **Début / Fin / Page Haut / Page Bas**. Beaucoup de laptops récents intègrent un pavé numérique complet mais, faute de place, en ont sacrifié le cluster de navigation dédié : pour l'atteindre, il faut alors jongler avec la touche **Fn** ou **MAJ**.

**Copilot Key+** remplace cette manipulation en pilotant le clavier au niveau logiciel, sans qu'aucun firmware n'ait besoin d'être modifié : Copilot devient la **touche MAGIQUE** qui manquait.

---

# ⌨️ Mon clavier, mes règles !!

Ce programme ne va pas changer le monde. Mais il m'a rendu quelque chose que je pensais avoir perdu : **LE SIMPLE PLAISIR** de taper sur mon clavier sans avoir à lutter contre lui. 💖

Plus de popup Copilot qui surgit intempestivement dès que trente ans de réflexes reprennent le dessus. Mon pavé numérique redevient disponible, avec `Début`, `Fin`, `Page Haut` et `Page Bas` à portée d'annulaire, en combinaison avec `Copilot`. Et ma touche `CTRL` retrouve enfin l'endroit où mes doigts s'attendent à la trouver.

Ce ne sont que de petites choses. Mais quand on a passé des années à construire des automatismes, ce n'est pas forcément à un fabricant de clavier ou à Microsoft de décider qu'il faut les réécrire pour nous.

Alors si votre clavier vous résiste lui aussi depuis que quelqu'un, quelque part, a décidé qu'un raccourci vers un assistant I.A était plus important que vos propres réflexes, ce petit programme est peut-être aussi pour vous.

**Gratuit, sans installation compliquée, et sans avoir à demander la permission à personne.** 🎉

---

## 📥 Installation

Copilot Key+ s'installe avec 📥 **[`Copilot Key+ - Install.exe`](https://github.com/Captain-FLAM/Copilot-Key-Plus/releases/latest/download/Copilot%20Key%2B%20-%20Install.exe)** (dernière release du dépôt) : choix de la langue, licence, dossier de destination (directement dans votre dossier utilisateur, `%UserProfile%`, par défaut - aucun droit administrateur requis), puis une case à cocher pour activer ou non le démarrage automatique avec Windows (clé de registre `Run`, cochée par défaut).

Une fois les fichiers copiés, un écran dédié propose de lancer la configuration : elle guide la capture de la signature Copilot propre à la machine (voir plus haut), le choix du comportement de la touche, et l'activation ou non du remap des flèches. Vous pouvez la relancer à tout moment sans tout réinstaller, via le raccourci **Configurer Copilot Key+** ajouté au menu Démarrer.

Pour arrêter proprement l'instance résidente, sans passer par le Gestionnaire des tâches, utilisez le raccourci **Quitter Copilot Key+** du menu Démarrer (ou `Copilot_Key+.exe -quit` depuis son dossier d'installation).

Pour désinstaller, utilisez le raccourci **Désinstaller Copilot Key+** du menu Démarrer ou le bouton dédié dans *Applications installées* (Paramètres Windows) : les deux relancent l'installateur, qui détecte l'installation existante et propose alors de la **réparer** ou de la **désinstaller**, plutôt que de repartir de zéro. L'installateur se copie lui-même dans le dossier d'installation à la fin du processus, donc pas besoin de conserver le fichier téléchargé - il n'y a pas de fichier séparé pour la désinstallation.

> ⚠️ **Touche Copilot matérielle dédiée (non testé)** : certains claviers très récents (Windows 11 23H2 et plus) embarquent une touche Copilot **matérielle dédiée**, distincte de la salve de touches classique (`Win+C`, `Win+Maj+F23`, etc.). Ce cas est géré en repli automatique lors de l'installation, mais je n'ai pas eu l'occasion de le tester sur un clavier équipé de cette touche. Si `-config` tourne en boucle sur "Aucune touche détectée", ou si le comportement n'est pas celui attendu sur ce type de clavier, n'hésitez pas à ouvrir une issue sur le dépôt.

---

## 🔀 Les combinaisons de touches

Ces choix se font à l'installation et sont re-modifiables à tout moment :

### Copilot (seule)

| Réglage choisi | Résultat |
| --- | --- |
| 1) CTRL droit *(par défaut)* | Copilot ⇒ ⌃ **CTRL droit** |
| 2) Menu contextuel | Copilot ⇒ 📋 **Menu contextuel** |
| 3) Désactivée | Copilot ⇒ 🚫 **Rien** |


Le remap des flèches (Copilot maintenu + flèche) ci-dessous est un réglage **indépendant** de ce choix.  
Il change complètement le résultat de `Copilot + Flèche`.  

*(Avec `Menu contextuel` ou `Désactivée` à la place de `CTRL droit`, ces combinaisons n'ont pas d'effet particulier : la flèche se comporte normalement, comme si Copilot n'existait pas.)*


### 1) Si le remap des flèches est **activé**

#### Copilot + Flèches

Tant que Copilot est maintenu enfoncé :

| Combinaison | Résultat |
| --- | --- |
| Copilot + ⬅️ | 🏠 **Début** |
| Copilot + ➡️ | 🏁 **Fin** |
| Copilot + ⬆️ | ⏫ **Page Haut** |
| Copilot + ⬇️ | ⏬ **Page Bas** |

#### Copilot MULTI

Copilot ne fait que transformer la flèche en Début/Fin/Page Haut/Page Bas :

si **MAJ** et/ou **CTRL** sont maintenues en plus (physiquement, sur le clavier), elles continuent d'être transmises normalement à Windows et se combinent avec ce résultat - exactement comme sur un clavier disposant d'un vrai cluster de navigation dédié.

Les combinaisons se multiplient donc naturellement :

| Combinaison | Résultat |
| --- | --- |
| MAJ + Copilot + ⬅️ | 🔤 Sélection jusqu'au début de la ligne |
| MAJ + Copilot + ➡️ | 🔤 Sélection jusqu'à la fin de la ligne |
| MAJ + Copilot + ⬆️ | 🔤 Sélection sur la page précédente |
| MAJ + Copilot + ⬇️ | 🔤 Sélection sur la page suivante |
| CTRL + Copilot + ⬅️ | 📄 Tout début du document |
| CTRL + Copilot + ➡️ | 📄 Toute fin du document **(Note \*)** |
| CTRL + MAJ + Copilot + ⬅️ | 📄 Sélection jusqu'au tout début du document |
| CTRL + MAJ + Copilot + ➡️ | 📄 Sélection jusqu'à la toute fin du document |

*(Ce sont les raccourcis standards de Windows - MAJ = sélection, CTRL = document entier - le résultat exact dépend donc de l'application active, comme sur n'importe quel clavier.)*

**Note \*** : Cette combinaison fonctionne sur mon clavier, mais à condition de taper :  
CTRL (normalement) + **[** Copilot ➡️ **]** à la vitesse de l'éclair !!

> Ce n'est pas un bug : c'est dû au contrôleur matériel du clavier. Et je n'ai pas encore trouvé de méthode de contournement...

### 2) Remap des flèches **désactivé**

Copilot se comporte alors uniquement comme la touche `CTRL droit` - les flèches ne sont plus transformées, Copilot agit comme un simple modificateur.

#### Copilot + Flèches

| Combinaison | Équivaut à | Résultat |
| --- | --- | --- |
| Copilot + ⬅️ | CTRL + ⬅️ | 🔤 Mot précédent |
| Copilot + ➡️ | CTRL + ➡️ | 🔤 Mot suivant |
| Copilot + ⬆️ | CTRL + ⬆️ | 🔤 Défilement vers le haut |
| Copilot + ⬇️ | CTRL + ⬇️ | 🔤 Défilement vers le bas |

*(Ctrl+Haut/Bas dépend un peu plus de l'application que Ctrl+Gauche/Droite : défilement dans la plupart des éditeurs de texte, parfois sans effet ailleurs.)*

#### Copilot MULTI

Les touches **MAJ** et/ou **CTRL** maintenues physiquement en plus continuent de se combiner normalement, exactement comme avec `CTRL droit` seul :

| Combinaison | Équivaut à | Résultat |
| --- | --- | --- |
| MAJ + Copilot + ⬅️ | MAJ + CTRL + ⬅️ | 🔤 Sélection du mot précédent |
| MAJ + Copilot + ➡️ | MAJ + CTRL + ➡️ | 🔤 Sélection du mot suivant |

---

> ⚠️ **Important - ordre d'appui pour `MAJ + Copilot + Flèche`** *(s'applique quel que soit le réglage ci-dessus)* : sur certains claviers, la salve matérielle de la touche Copilot inclut elle-même la touche `MAJ` (exemple : `Win + Maj + F23`). Le programme ne peut alors pas distinguer un vrai appui sur `MAJ` du bruit matériel de la salve - même scancode. Pour que `MAJ + Copilot + Flèche` fonctionne sur ces claviers, il faut donc **appuyer sur `MAJ` avant Copilot**, et non l'inverse : maintenez `MAJ`, *puis* appuyez sur Copilot, *puis* sur la flèche. Si l'ordre est inversé, `MAJ` sera silencieusement ignoré.
>
> ⚠️ Sur certains claviers, une combinaison `CTRL + Copilot + Flèche` avec la touche `Copilot` **maintenue longtemps** avant d'appuyer sur la flèche peut occasionnellement ne pas être détectée (collision matérielle avec la répétition de la salve Copilot) - appuyez sur `Copilot` puis **juste après** sur **la flèche rapidement** pour que ça fonctionne correctement.

---

## 🕰️ Un mot sur ce projet

Ce petit programme est le fruit de plusieurs **mois** de recherches et de tests éparpillés - comprendre comment Windows expose ces touches, mesurer les salves de scancodes propres à chaque fabricant, essayer, rater, mettre de côté, revenir dessus des semaines plus tard... tellement épuisé...

Et puis un jour, je m'y suis remis sérieusement, épaulé par **Claude.ai** dans **VS Code** - et tout ce qui traînait depuis des mois a enfin été nettoyé, consolidé et finalisé en 6 jours de travail acharné ! 🚀

## ⭐ Ça vous a plu ?

Si Copilot Key+ vous a rendu, à vous aussi, un peu de plaisir à taper sur votre clavier, **mettez une Étoile** ⭐ sur ce dépôt : c'est gratuit, ça prend deux secondes, et ça aide d'autres claviers martyrisés à tomber dessus.

Et si vous connaissez quelqu'un dont la touche `Copilot` fait le désespoir, parlez-en autour de vous - un lien partagé, une mention en passant, ou simplement le bouche-à-oreille : ***« Faites passer le mot ! »*** , chaque victoire compte contre **les méfaits de Microsoft**. 😄

---

## 📜 Licence

© Captain FLAM - 2026

Ce projet est distribué sous licence **MIT**. En résumé : vous êtes libre de l'utiliser, le copier, le modifier, le fusionner, le publier, le distribuer et même en vendre des copies, à titre gratuit ou payant, à condition de conserver la mention de copyright et cet avis de licence dans toutes les copies ou parties substantielles du logiciel.

Le logiciel est fourni "tel quel", sans garantie d'aucune sorte, explicite ou implicite, y compris - mais sans s'y limiter - les garanties de qualité marchande, d'adéquation à un usage particulier et d'absence de contrefaçon. En aucun cas les auteurs ne pourront être tenus responsables d'une réclamation, de dommages ou d'une autre responsabilité, que ce soit dans le cadre d'un contrat, d'un délit ou autre, découlant de, ou en relation avec, le logiciel ou son utilisation ou d'autres transactions le concernant.

Icône libre de droits, usage commercial autorisé sans attribution : [uxwing.com/copilot-icon](https://uxwing.com/copilot-icon/)

---
