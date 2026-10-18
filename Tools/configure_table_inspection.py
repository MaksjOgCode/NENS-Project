"""Import the table inspection art and connect the existing BP_Table. Run with UE closed.

Existing textures and scene data are reused without resetting their editable layout.
Only BP_Table and this scene's new resources are saved; no test level is modified.
"""
import json
from pathlib import Path
import unreal

out = Path(unreal.Paths.project_dir()) / 'Saved' / 'TableInspection'
out.mkdir(parents=True, exist_ok=True)
source = Path('C:/Users/laqks/OneDrive/Desktop/character')
texture_folder = '/Game/NENS/UI/Inspection/Textures'
tools = unreal.AssetToolsHelpers.get_asset_tools()


def texture(name):
    path = texture_folder + '/' + name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        asset = unreal.load_asset(path)
        assert isinstance(asset, unreal.Texture2D), path
        return asset
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', str(source / (name + '.png')))
    task.set_editor_property('destination_path', texture_folder)
    task.set_editor_property('destination_name', name)
    task.set_editor_property('automated', True)
    task.set_editor_property('save', False)
    tools.import_asset_tasks([task])
    asset = unreal.load_asset(path)
    assert isinstance(asset, unreal.Texture2D), 'Texture import failed: ' + path
    asset.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_UI)
    asset.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    asset.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    asset.set_editor_property('srgb', True)
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset), path
    return asset


background = texture('T_Inspection_Table')
note = texture('T_Note')
scene_path = '/Game/NENS/UI/Inspection/DA_Inspection_Table'
scene = unreal.load_asset(scene_path) if unreal.EditorAssetLibrary.does_asset_exist(scene_path) else None
if scene is None:
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', unreal.InspectionSceneData)
    scene = tools.create_asset('DA_Inspection_Table', '/Game/NENS/UI/Inspection', unreal.InspectionSceneData, factory)
    assert scene, 'Scene data creation failed'
    scene.set_editor_property('background', background)
    scene.set_editor_property('canvas_size', unreal.Vector2D(2001, 1561))
    pickup = unreal.InspectionPickup()
    pickup.set_editor_property('pickup_id', 'Table_Note')
    pickup.set_editor_property('item_id', 'Note')
    pickup.set_editor_property('image', note)
    pickup.set_editor_property('position', unreal.Vector2D(540, 850))
    pickup.set_editor_property('size', unreal.Vector2D(328, 384.5))
    scene.set_editor_property('pickups', [pickup])
    assert unreal.EditorAssetLibrary.save_loaded_asset(scene), scene_path
assert isinstance(scene, unreal.InspectionSceneData), scene_path
table = unreal.load_asset('/Game/NENS/Environment/Interior/Blueprints/BP_Table')
assert table, 'BP_Table missing'
result = json.loads(unreal.InspectionEditorLibrary.configure_searchable_actor(table, scene))
result['scene'] = scene_path
(out / 'setup-result.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
assert result.get('success'), result
(out / 'BP_Table-configured.json').write_text(unreal.InspectionEditorLibrary.dump_blueprint(table), encoding='utf-8')
unreal.log('NENS_TABLE_INSPECTION_OK ' + json.dumps(result))
