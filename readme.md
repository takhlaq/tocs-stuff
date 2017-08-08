jp/en audio toggle for Trails of Cold Steel

https://github.com/takhlaq/tocs-audio-toggle

**usage:**
- buy the game https://store.steampowered.com/app/538680/The_Legend_of_Heroes_Trails_of_Cold_Steel/ (GOG version is untested but should work fine also)
- grab vs2015 x86 runtime https://www.microsoft.com/en-gb/download/details.aspx?id=48145
- place jp audio files in `Trails of Cold Steel/data/voice/wav_jp/` (e.g. file path should look like `/data/voice/wav_jp/e8v00000.wav`)
- launch Trails of Cold Steel
- launch tocs-injector from ed8.exe/ed8_jp.exe directory (may need to run as admin)
- hit enter in tocs-injector window with no input entered to toggle english/jp

**issues:**
- `what is this shit where's my fancy gui and kb shortcuts?` - in your pull request, go make one
- `your code is garbage` - ungarbage it with pull request (actual code review is appreciated though!)
- `audio wont change back/cant delete the dll` - failed to unload for some reason, try rebooting (if you injected the dll with cheatengine for whatever reason, close cheatengine)

**todo:**
- probably use imgui overlay if i ever lrn2gui since its sitting in submodules anyways
- get rid of this shitty commandline crap and just load the dll itself