# Blob Survival
A small chaotic vampire-survivors inspired game written in C and raylib. It uses a quad-tree for effecient collisions and is capable of handling thousands of enemies.

You can play a wasm build of the game here - [https://bones-ai.itch.io/blob-survival](https://bones-ai.itch.io/blob-survival)

![untitled2(3)](https://github.com/user-attachments/assets/417b7127-6406-4429-8f7a-973c52bdc935)


# Small demo on YT
[![youtube](https://img.youtube.com/vi/kHefr2VUDvw/0.jpg)](https://youtu.be/kHefr2VUDvw)


# Read this
- You won't readily be able to run the code because I'm not allowed to redistribute some of the assets used in the game, i've put in some placeholder assets for you to replace.
- The entire game is within a single `game.c` file, I use `:` tags to quickly jump between different sections, I find this to be more effective than splitting it into separate files, and there's also some `// MARK: ` that helps if you're using vscode
- This is the trimmed down version of readme file I used to document my todo and other things while building the game
- Refer to the end of the file for credits and links to assets and other resources I used
- Use the `z_build.sh` to run the game

# Done
- clean up the heartbeat audio noise
- reduce the spawn duration
- have a internal volume thingy that allows for the heartbeat to be heard easily over all other sounds
- hurt sounds for enemy bullets
- hurt sound effect
- heart pickup sound effect
- reduce the number of heart pickups
- disable the escape key behaviour on release builds
- bug: spike toast always says unlocked, flame always upgraded
- heart beat music when the health drops below a certain value
- Clean up old enemies that are far behind
- Update music for frost wave
- Music/Sounds 2, on upgrade, 
    - splinter new music
    - frost wave
    - orbs
    - upgrade
- Make the main menu fancy?
- Pause menu with stats and other stuff and redirect to shop menu?
- More upgrades like pickup range etc. ?
- UI for when a weapon will shoot (-)
- Lazer attack (-)
- some simple analytics for when someone plays the game?
- Adjust the number of particles for the area attacks based on upgrades
- Different shiny enemies (-)
- Enemy drop different value of mana (-)
- More pickup types
- Some way to clean up the pickups that are too old
- Make enemy bullets collide with player
- Color pallete change?
- Player takes damage
- Make pickup range a circle
- Upgrades post a point will be random
- Volume controls in the pause menu
- Settings menu with volume controls
- Global gamestate
- Save system (-)
- Change the clock to a distance based thing (-)
- Toast system
- Main menu
- New upgrade system
- TOWN (no town for now)
- (didn't think i need this) Make gamestate init happen on game start, and have a pregamestate for before the game starts?
- Music/Sounds
- Refactor
- Camera system
- Fix enemy spawning around player
- Quadtree for collisions
- Single file arch
- Fixed map size, clamp player within map
- Random map decorations
- Custom font
- Draw map borders
- Simple enemy rampup system
- Pixelate everything
- Letter boxing ie black padding and resize handling
- Virtual joystick, mobile and touch controls
- Decorations again
- Shiny enemies
- Different attack types
- Enemy drop loot
- Track picked up mana and display a progress bar that fills up
- Level up as you collect mana, each level up needs higher mana
- Upgrade system on each level up
- Particle system
- Animations? (like collecting pickups)
- Sprite x flip and sprite bob
- Frost wave and slowing enemies down
- Enemy variants, Enemies with better health/values
- Unique enemy behaviours
    - slime - one shotable, def speed, nothing special, default spawn
    - deer - fast, low health, charge at you?, 0.2 prob @ 1-2 mins?
    - emu - med speed, fires bullets, med health, 0.2 prob @ 5 mins
    - dino - spawns pups, fires bullets, 0.1 prob @ 10 mins, max of 5 can exist

# Optimisations
- Clean up enemies that are too far away
- Stop updating enemies out of the screen?
- Don't boid enemies outside the screen
- Music files in ogg have lower size, idk if converting everything to ogg is better

# Other things
- learn lighting?
- Blood splats
- Fog for far away enemies?
- Make boid separation based on a value for each enemy, shiny and important enemies should be prioritized?

# Crashes
- When you have a lot of enemies, we get this crash, this should technically not happen since the player can't be alive for that long ???
    - To replicate the crash, spawn a lot of enemies and stand inplace
    - This happens when the qtree is reset with points
    - Potential cause - the quadtree is oversplit at the player pos to the point that the boundary becomes invalid? idk
    ```
    Process 18590 stopped
    * thread #1, queue = 'com.apple.main-thread', stop reason = EXC_BAD_ACCESS (code=2, address=0x16f603ff8)
        frame #0: 0x000000010000373c game_debug`is_rect_contains_point(rect=(x = <read memory from 0x16f603ff8 failed (0 of 4 bytes read)>, y = <read memory from 0x16f603ffc failed (0 of 4 bytes read)>, w = 0, h = 0), pt=(x = <read memory from 0x16f603fe8 failed (0 of 4 bytes read)>, y = <read memory from 0x16f603fec failed (0 of 4 bytes read)>, id = <read memory from 0x16f603ff0 failed (0 of 4 bytes read)>)) at game.c:876
    873
    874 	// MARK: :utils
    875
    -> 876 	bool is_rect_contains_point(QRect rect, QPoint pt) {
    877 	    return (pt.x >= rect.x - rect.w && pt.x <= rect.x + rect.w) &&
    878 	           (pt.y >= rect.y - rect.h && pt.y <= rect.y + rect.h);
    879 	}
    Target 0: (game_debug) stopped
    ```
- The pickups qtree range is 32k, anything beyond that wont be recognized

# Ideas
- For new attacks
    - Orbs (blobs of light that scatter around randomly)
    - Rockets - just fire random bullets that do area damage
    - Lasers
    - Bomb, drop inplace and it blows up after awhile
    - Meteors
    - Yoyo like attack? spawns and comes back to the player
    - A giant ball stuck to player with a chain
    - Fire trail? or trails in general
- Utility types
    - Shield, Invinsibility,

# Links
- Raylib wasm build - https://www.youtube.com/watch?v=j6akryezlzc
- emcc to run at 120fps, from karl odin raylib wasm template - https://github.com/karl-zylinski/odin-raylib-web
- Lospec pallete - https://lospec.com/palette-list/laser-lab
- new color pallete - https://lospec.com/palette-list/joker-6
- Quadtree CodingTrain - https://www.youtube.com/watch?v=OJxEcs0w_kE, code - https://editor.p5js.org/codingtrain/sketches/g7LnWQ42x
- Alagard font by Hewett Tsoi - https://www.dafont.com/alagard.font
- Boid separation - https://www.youtube.com/watch?v=mhjuuHl6qHM
- Pixel camera - https://github.com/raysan5/raylib/blob/master/examples/core/core_smooth_pixelperfect.c, demo - https://www.raylib.com/examples/core/loader.html?name=core_smooth_pixelperfect
- Raylib letterboxing (black bars around the viewport when window is resized) - https://github.com/raysan5/raylib/blob/master/examples/core/core_window_letterbox.c, demo - https://www.raylib.com/examples/core/loader.html?name=core_window_letterbox
- Enemy assets - https://cmski.itch.io/tabletop-armies-asset-pack
- Grass & Stone assets - https://egordorichev.itch.io/toi
- Player assets - https://ibirothe.itch.io/roguelike1bit16x16assetpack
- bg music - https://makotohiramatsu.itch.io/north-sea
- impact sounds - https://kenney.nl/assets/impact-sounds
- sounds - https://opengameart.org/content/fire-crackling, https://opengameart.org/content/catching-fire, https://pixabay.com/sound-effects/flamethrower-19895/
- heartbeat - https://pixabay.com/sound-effects/heartbeat-loud-242421/, https://pixabay.com/sound-effects/heartbeat-loop-96879/
- buildings in town - https://iknowkingrabbit.itch.io/rts-micro-asset
- random sprites from urizen one bit sprite sheet - https://vurmux.itch.io/urizen-onebit-tileset
- more sound effects - https://brainzplayz.itch.io/retro-sounds-32-bit
- more sound effects - https://lmglolo.itch.io/free-fps-sfx
- more sound fx - https://kronbits.itch.io/freesfx
- more sound fx - https://pixabay.com/sound-effects/short-fireball-woosh-6146/
- heart beat fx - https://pixabay.com/sound-effects/heartbeat-loud-242421/
- brackeys-platformer-bundle - https://brackeysgames.itch.io/brackeys-platformer-bundle
