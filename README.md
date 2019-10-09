# ![logo](https://raw.githubusercontent.com/azerothcore/azerothcore.github.io/master/images/logo-github.png) AzerothCore
- Latest build status with azerothcore: [![Build Status](https://travis-ci.org/azerothcore/mod-guildhouse.svg?branch=master)](https://travis-ci.org/azerothcore/mod-guildhouse)
# Guild House Module


## Description

This is a phased guild house system for Azerothcore, it allows players to from the same guild to visit their guild house to explore, train ect
All guilds will get their own phasing system which then the guild master will have to purchase NPC's creatures and other stuff to complete the creation. 


## How to use ingame
Once a player has brought a guild house from the NPC they can either teleport to the guildhouse by via the NPC or do .guildhouse tele

Once the player is in the location of the guild house the guild master has a command .guildhouse spawnnpc this will allow the guild master
to start placing objects / npc within the guild house.

## Requirements

My new module requires:

- AzerothCore v1.0.1+

## Installation

```
1) Simply place the module under the `modules` directory of your AzerothCore source. 
2) Import the SQL manually to the right Database (auth, world or characters)
3) Apply the guildhouse.patch to your source 
4) Re-run cmake and launch a clean build of AzerothCore.
```
## Patch Information

The patch basicly turns the selected area from a bitmask to a uint, this fixes the issues with the phasing

Before Patch :
Guild 1 Can see phase 1
Guild 2 Can see phase 2
Guild 3 Can See Guild 1 & 2

After Patch: 
All guilds will now have a seperate phase for the area 876 (GM Island)

## Edit module configuration (optional)

If you need to change the module configuration, go to your server configuration folder (where your `worldserver` or `worldserver.exe` is), copy `mod_guild_house_v2.conf.dist` to `mod_guild_house_v2.conf` and edit that new file.

## Credits

* [Me](https://github.com/talamortis) (author of the module)
* [Rochet2](https://github.com/Rochet2/): Thanks for the help with the phasing situation & General support

AzerothCore: [repository](https://github.com/azerothcore) - [website](http://azerothcore.org/) - [discord chat community](https://discord.gg/PaqQRkd)
