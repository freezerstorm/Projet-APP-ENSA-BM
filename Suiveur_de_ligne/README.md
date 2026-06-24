# 🤖 Line Follower Robot — Arduino + PID

Robot suiveur de ligne à 4 moteurs avec correcteur PID, basé sur Arduino et le driver L298N.

---

## 📷 Aperçu

```
        [Capteur G]  [Capteur D]
               ↓         ↓
         ┌─────────────────┐
         │     Arduino     │
         │      Uno        │
         └────────┬────────┘
                  │
         ┌────────▼────────┐
         │     L298N       │
         └──┬──────────┬───┘
         [M.Gauche] [M.Droit]
```

---

## ⚙️ Matériel requis

| Composant | Quantité |
|---|---|
| Arduino Uno (ou compatible) | 1 |
| Driver moteur L298N | 1 |
| Moteurs DC + roues | 4 |
| Capteurs IR (ex: TCRT5000) | 2 |
| Châssis 4WD | 1 |
| Alimentation (7-12V) | 1 |
| Câbles, breadboard | — |

---

## 🔌 Câblage

### Capteurs IR → Arduino

| Capteur | Pin Arduino |
|---|---|
| Capteur Gauche (OUT) | D2 |
| Capteur Droit (OUT) | D4 |

> Les capteurs doivent renvoyer `HIGH` sur la ligne noire et `LOW` sur le fond blanc. Ajuste le potentiomètre du module IR si nécessaire.

### L298N → Arduino

| L298N | Pin Arduino |
|---|---|
| ENA | D3 (PWM) |
| IN1 | D8 |
| IN2 | D9 |
| ENB | D5 (PWM) |
| IN3 | D10 |
| IN4 | D11 |

---

## 🧠 Fonctionnement

### Logique de suivi

| Capteur Gauche | Capteur Droit | État | Action |
|:---:|:---:|---|---|
| 1 | 1 | Ligne centrée | Avance tout droit |
| 1 | 0 | Robot trop à droite | Correction gauche |
| 0 | 1 | Robot trop à gauche | Correction droite |
| 0 | 0 | Ligne perdue | Recherche 1.5s puis stop |

### Correcteur PID

Le robot utilise un correcteur **PID (Proportionnel - Intégral - Dérivé)** pour ajuster la vitesse de chaque côté de manière fluide :

```
Correction = Kp × erreur + Ki × intégrale + Kd × dérivée

vitesse_gauche = BASE_SPEED - correction
vitesse_droite = BASE_SPEED + correction
```

| Terme | Rôle |
|---|---|
| **Kp** | Réaction immédiate à l'écart de position |
| **Ki** | Corrige les dérives lentes et persistantes |
| **Kd** | Amortit les oscillations (anti-zigzag) |

### Mécanisme de récupération

Quand la ligne est perdue, le robot continue dans le **dernier sens connu** pendant 1,5 seconde avant de s'arrêter, ce qui permet de retrouver la ligne dans les virages serrés.

---

## 🚀 Installation

1. Clone ce dépôt :
```bash
git clone https://github.com/ton-utilisateur/line-follower-pid.git
```

2. Ouvre `line_follower_pid.ino` dans l'**IDE Arduino**

3. Sélectionne ta carte : `Outils > Type de carte > Arduino Uno`

4. Téléverse le code sur ta carte

---

## 🎛️ Calibration du PID

Les paramètres PID sont définis en haut du fichier :

```cpp
float Kp = 120.0;
float Ki = 0.0;
float Kd = 50.0;

#define BASE_SPEED 120
```

### Procédure recommandée

```
1. Commence avec Kp seul (Ki=0, Kd=0)
   → Augmente Kp jusqu'à ce que le robot suive mais commence à osciller

2. Augmente Kd pour éliminer les oscillations
   → Le robot doit suivre sans zigzaguer

3. Active Ki uniquement si le robot dérive systématiquement d'un côté
   → Commence avec Ki = 0.5 et ajuste doucement
```

### Valeurs de départ conseillées

| Paramètre | Valeur initiale | Effet si augmenté |
|---|---|---|
| `Kp` | 120 | Plus réactif mais peut osciller |
| `Ki` | 0 | Corrige les dérives (risque de windup) |
| `Kd` | 50 | Réduit les oscillations |
| `BASE_SPEED` | 120 | Plus rapide mais moins stable |

---

## 🐛 Dépannage

| Problème | Cause probable | Solution |
|---|---|---|
| Robot oscille en zigzag | `Kp` trop élevé | Réduire `Kp`, augmenter `Kd` |
| Robot sort des virages | Vitesse trop élevée | Réduire `BASE_SPEED` |
| Robot ne réagit pas assez | `Kp` trop faible | Augmenter `Kp` |
| Robot tire d'un côté | Moteurs déséquilibrés ou dérive | Activer `Ki` légèrement |
| Ligne perdue en permanence | Mauvais réglage capteurs IR | Ajuster le potentiomètre du module IR |

### Debug via le moniteur série

Ouvre le moniteur série à **9600 baud** pour visualiser les valeurs en temps réel :

```
err:-1.00 corr:-120.00 L:240 R:0
err:0.00  corr:0.00    L:120 R:120
err:1.00  corr:120.00  L:0   R:240
```

---

## 📁 Structure du projet

```
line-follower-pid/
│
├── line_follower_pid.ino   # Code principal Arduino
└── README.md               # Ce fichier
```

---

## 🔧 Améliorations possibles

- [ ] Ajouter 3 capteurs supplémentaires (5 au total) pour une erreur plus précise (-4 à +4)
- [ ] Calibration automatique des capteurs au démarrage
- [ ] Réglage des paramètres PID via le port série sans re-flasher
- [ ] Affichage LCD pour visualiser les paramètres en direct
- [ ] Mode apprentissage du circuit

---

## 📄 Licence

Ce projet est sous licence **MIT** — libre d'utilisation, modification et distribution.

---

## 👤 Auteur

Fait avec ❤️ par **Manu**
