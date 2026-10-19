"""Fix the offset interaction-distance bug and the Gosling picture's ring depth.

One-time Unreal editor setup. Other furniture, the map, textures and materials are not saved.
"""
import json
from pathlib import Path
import unreal

out = Path(unreal.Paths.project_dir()) / 'Saved/InteractionGeometry'
out.mkdir(parents=True, exist_ok=True)
result = json.loads(unreal.InspectionEditorLibrary.fix_interaction_geometry())
(out / 'setup-result.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
assert result.get('success'), result
for path in ['/Game/NENS/Gameplay/Interaction/Components/BPC_Interaction', '/Game/NENS/Environment/Interior/Blueprints/BP_Furniture_Gosling']:
    bp = unreal.load_asset(path)
    validation = json.loads(unreal.InspectionEditorLibrary.compile_and_save(bp, False))
    assert validation['success'] and validation['warnings'] == 0, validation
    (out / (path.rsplit('/',1)[-1] + '-after.json')).write_text(unreal.InspectionEditorLibrary.dump_blueprint(bp), encoding='utf-8')
unreal.log('NENS_INTERACTION_GEOMETRY_FIXED')
