"""One-time editor migration; normal Blueprint editing does not need this script."""
import json
from pathlib import Path
import unreal

result = json.loads(unreal.InspectionEditorLibrary.configure_valves_minigame())
out = Path(unreal.Paths.project_saved_dir()) / 'ValvesIntegration'
out.mkdir(exist_ok=True)
(out / 'configure-result.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
assert result.get('success'), result
for path in [
    '/Game/MiniGameValves2/WBP_valves1',
    '/Game/MiniGameValves2/WBP_ValvesBoard',
    '/Game/NENS/Environment/Interior/Props/Blueprints/BP_Props_PickMeBox',
    '/Game/Minigames/Wires/WBP_WiresMiniGame',
]:
    bp = unreal.load_asset(path)
    check = json.loads(unreal.InspectionEditorLibrary.compile_and_save(bp))
    assert check.get('success'), (path, check)
    (out / (path.rsplit('/',1)[-1] + '-after.json')).write_text(
        unreal.InspectionEditorLibrary.dump_blueprint(bp), encoding='utf-8')
unreal.log('NENS_VALVES_CONFIGURE_OK')
