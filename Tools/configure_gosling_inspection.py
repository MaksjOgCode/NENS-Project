"""Connect the existing Gosling picture to a repeatable, empty inspection scene.

Reuses the user's Blueprint and texture; no import, level edit or runtime change.
Run through an Unreal Python commandlet with the interactive editor closed.
"""
import json
from pathlib import Path
import unreal

out = Path(unreal.Paths.project_dir()) / 'Saved' / 'GoslingInspection'
out.mkdir(parents=True, exist_ok=True)
background = unreal.load_asset('/Game/NENS/Environment/Interior/Textures/T_Inspection_Gosling')
picture = unreal.load_asset('/Game/NENS/Environment/Interior/Blueprints/BP_Furniture_Gosling')
assert isinstance(background, unreal.Texture2D) and picture, 'Existing Gosling picture assets are missing'
path = '/Game/NENS/UI/Inspection/DA_Inspection_Gosling'
scene = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
if scene is None:
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', unreal.InspectionSceneData)
    scene = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        'DA_Inspection_Gosling', '/Game/NENS/UI/Inspection', unreal.InspectionSceneData, factory)
    assert scene, 'Could not create Gosling scene data'
    scene.set_editor_property('background', background)
    scene.set_editor_property('canvas_size', unreal.Vector2D(
        background.blueprint_get_size_x(), background.blueprint_get_size_y()))
    scene.set_editor_property('pickups', [])
    assert unreal.EditorAssetLibrary.save_loaded_asset(scene), path
assert isinstance(scene, unreal.InspectionSceneData) and not scene.pickups, 'Hint-only scene must have no pickups'
result = json.loads(unreal.InspectionEditorLibrary.configure_searchable_actor(picture, scene))
result.update({'scene': path, 'pickups': 0, 'repeatable': True})
(out / 'setup-result.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
assert result.get('success'), result
compile_result = json.loads(unreal.InspectionEditorLibrary.compile_and_save(picture, False))
(out / 'blueprint-validation.json').write_text(json.dumps(compile_result, indent=2), encoding='utf-8')
assert compile_result['success'] and compile_result['warnings'] == 0, compile_result
(out / 'BP_Furniture_Gosling-after.json').write_text(unreal.InspectionEditorLibrary.dump_blueprint(picture), encoding='utf-8')
unreal.log('NENS_GOSLING_INSPECTION_OK ' + json.dumps(result))
