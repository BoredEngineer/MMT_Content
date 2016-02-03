# MMT_Content
Machinery Modelling Toolkit is a plugin for UE4. This plugin provides some basic means to add custom physics code in blueprints, which can be executed during physics sub-stepping. This repository contains both plugin and content examples of how it can be used.

The basis of the plugin is a custom pawn MMT_Pawn, which has a custom event "MMT_Physics_Tick" executed during normal and sub-stepped physics updates. In addition to MMT_Pawn you will find several functions which a necessary to query and interact with objects during physics sub-stepping. All functions have prefix "MMT" so it's easy to find them among blueprint nodes.

More information can be found here:
https://forums.unrealengine.com/showthread.php?83483-ASSETS-OPEN-SOURCE-Tanks-tracks-and-N-wheeled-vehicles
