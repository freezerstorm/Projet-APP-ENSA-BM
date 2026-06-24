# ⚡ V2V Energy Transfer — Voiture Immobile (Arduino)

Système de transfert d'énergie **Vehicle-to-Vehicle (V2V)** pour la voiture stationnaire. Ce module gère automatiquement le basculement entre le mode **Onduleur** (émission d'énergie) et le mode **Redresseur** (réception d'énergie) en fonction des tensions mesurées, avec affichage LCD et contrôle moteur.

---

## 📷 Aperçu du système

```
  [ Batterie / Source ]
          │
     ┌────▼────┐        Bobine         ┌─────────────┐
     │ Arduino │ ──── ( Transfert ) ── │ Voiture     │
     │  Uno    │       inductif        │ Mobile      │
     └────┬────┘                       └─────────────┘
          │
   ┌──────┼──────┐
[Relais] [LCD] [Moteur]
```

---

## ⚙️ Matériel requis

| Composant | Quantité |
|---|---|
| Arduino Uno (ou compatible) | 1 |
| Écran LCD I2C 16×2 (adresse 0x27) | 1 |
| Relais 5V (active LOW) | 3 |
| Pont diviseur de tension (mesure) | 2 |
| Moteur DC | 1 |
| Driver moteur (L298N ou similaire) | 1 |
| Fin de course (interrupteur) | 1 |
| Câbles, breadboard | — |

---

## 🔌 Câblage

### Relais → Arduino

| Relais | Pin Arduino | Rôle |
|---|---|---|
| `Relay_ondo_Bat_redro` | D2 | Bascule Onduleur / Batterie-Redresseur |
| `Relay_ondo_Bob_redro` | D3 | Bascule Onduleur / Bobine-Redresseur |
| `Relay_Ground` | D4 | Commutation masse |

> ⚠️ Les relais sont **ACTIVE LOW** : `LOW` = relais activé, `HIGH` = relais désactivé.

### Moteur → Arduino

| Signal | Pin Arduino |
|---|---|
| IN1 (sens 1) | D7 |
| IN2 (sens 2) | D8 |

### Autres connexions

| Composant | Pin Arduino |
|---|---|
| Fin de course | D12 (INPUT_PULLUP) |
| PWM (onduleur) | D9 (Timer1) |
| Tension propre | A0 |
| Tension secondaire | A1 |
| LCD SDA | A4 |
| LCD SCL | A5 |

---

## 🧠 Fonctionnement

### Deux modes automatiques

| Mode | Condition de déclenchement | Action |
|---|---|---|
| **ONDULEUR** | `tension_propre > tension_second + 0.5V` | Génère un signal AC via PWM → transfère l'énergie |
| **REDRESSEUR** | `tension_propre < tension_second - 0.5V` | Reçoit et redresse l'énergie de la voiture mobile |

### Basculement automatique avec hystérésis

```
tension_propre > tension_second + 0.5V  →  MODE ONDULEUR
tension_propre < tension_second - 0.5V  →  MODE REDRESSEUR
```

L'hystérésis de **0.5V** évite les basculements intempestifs quand les deux tensions sont proches.

### Génération du signal PWM (Onduleur)

Le PWM est généré en **mode matériel** via le Timer1 de l'Arduino (indépendant du `loop`) :

```
Fréquence = F_CPU / (prescaler × (ICR1 + 1))
          = 16 000 000 / (1 × 800)
          = 20 000 Hz  →  20 kHz
Rapport cyclique = OCR1A / ICR1 = 400 / 799 ≈ 50%
```

### Mesure de tension

Les tensions sont lues via un pont diviseur et converties avec le facteur de calibration :

```
tension_reelle = (ADC × 5.0 / 1023) × 4.4531835
```

### Contrôle moteur

Le moteur change de sens selon le mode actif, et s'arrête automatiquement si le **fin de course** est déclenché (`LOW`) :

| Mode | Sens moteur |
|---|---|
| ONDULEUR | Sens 2 (IN1=LOW, IN2=HIGH) |
| REDRESSEUR | Sens 1 (IN1=HIGH, IN2=LOW) |
| Fin de course déclenché | Stop |

---

## 🔄 Séquence de démarrage

```
1. Initialisation des pins
2. Affichage "Systeme V2V / Initialisation" (2s)
3. Configuration du Timer1 (PWM 20kHz)
4. Boucle principale (toutes les 500ms) :
   ├── Lecture tensions A0 et A1
   ├── Basculement automatique de mode
   ├── Contrôle du moteur
   └── Mise à jour LCD
```

---

## 📺 Affichage LCD

```
┌────────────────┐
│ ONDULEUR       │   ← Mode actif
│ P:12.4  S:8.1  │   ← Tension propre / secondaire (V)
└────────────────┘
```

```
┌────────────────┐
│ REDRESSEUR     │
│ P:7.8   S:13.2 │
└────────────────┘
```

---

## 📡 Moniteur Série

Ouvre le moniteur série à **9600 baud** pour suivre les tensions en temps réel :

```
P = 12.43 V | S = 8.12 V
P = 11.98 V | S = 8.20 V
P = 8.15 V  | S = 13.40 V
```

---

## 🎛️ Paramètres ajustables

```cpp
const float HYSTERESIS = 0.5;        // Seuil de basculement (V)
const float facteur    = 4.4531835;  // Facteur de calibration pont diviseur
```

| Paramètre | Effet si augmenté |
|---|---|
| `HYSTERESIS` | Basculement moins fréquent, plus stable |
| `facteur` | Recalibrer si la tension affichée est incorrecte |
| `ICR1` | Modifier la fréquence PWM |
| `OCR1A` | Modifier le rapport cyclique PWM |

---

## 🐛 Dépannage

| Problème | Cause probable | Solution |
|---|---|---|
| Tension affichée incorrecte | Facteur de calibration | Mesurer avec un voltmètre et ajuster `facteur` |
| Basculement trop fréquent | Hystérésis trop faible | Augmenter `HYSTERESIS` (ex: 1.0V) |
| LCD n'affiche rien | Mauvaise adresse I2C | Tester avec `0x3F` au lieu de `0x27` |
| Moteur ne tourne pas | Fin de course bloqué | Vérifier l'état de `FIN_COURSE` (D12) |
| PWM ne démarre pas | Conflit Timer1 | Ne pas utiliser `analogWrite` sur D9/D10 |
| Relais ne bascule pas | Logique inversée | Rappel : `LOW` = activé, `HIGH` = désactivé |

---

## 📁 Structure du projet

```
V2V-voiture-immobile/
│
├── voiture_immobile.ino   # Code principal Arduino
└── README.md              # Ce fichier
```

---

## 🔗 Projet lié

Ce code fait partie d'un système **V2V complet** composé de deux modules :

| Module | Rôle |
|---|---|
| **Voiture Immobile** (ce repo) | Gestion Onduleur / Redresseur + LCD |
| **Voiture Mobile** | Réception / émission d'énergie côté mobile |

---

## 🔧 Améliorations possibles

- [ ] Ajout d'un régulateur PID sur le PWM pour stabiliser le transfert
- [ ] Communication Bluetooth entre les deux voitures
- [ ] Mesure du courant pour calculer la puissance transférée
- [ ] Historique des tensions sur carte SD
