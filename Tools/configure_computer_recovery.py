"""Connect the existing puzzles and collectible definitions to one recovery goal."""
import json
from pathlib import Path
import unreal

out = Path(unreal.Paths.project_saved_dir())/'ComputerRecovery'
out.mkdir(parents=True, exist_ok=True)
tools = unreal.AssetToolsHelpers.get_asset_tools()
folder = '/Game/NENS/UI/Recovery'
data_path = folder+'/DA_ComputerRecovery'
if unreal.EditorAssetLibrary.does_asset_exist(data_path):
    data = unreal.load_asset(data_path)
else:
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', unreal.ComputerRecoveryData)
    data = tools.create_asset('DA_ComputerRecovery', folder, unreal.ComputerRecoveryData, factory)
    # All current punch-card scenes, found from their actual ItemID definitions.
    scenes = []
    for path in unreal.EditorAssetLibrary.list_assets('/Game/NENS/UI/Inspection', recursive=True, include_folder=False):
        asset = unreal.load_asset(path)
        if isinstance(asset, unreal.InspectionSceneData) and any(str(p.item_id)=='PunchCard' for p in asset.pickups):
            scenes.append(asset)
    assert scenes, 'No current punch-card scenes found'
    data.set_editor_property('card_scenes', sorted(scenes, key=lambda s:s.get_path_name()))
    assert unreal.EditorAssetLibrary.save_loaded_asset(data)
widget_path = folder+'/WBP_ComputerRecovery'
if not unreal.EditorAssetLibrary.does_asset_exist(widget_path):
    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property('parent_class', unreal.ComputerRecoveryWidget)
    assert tools.create_asset('WBP_ComputerRecovery', folder, unreal.WidgetBlueprint, factory)
result = json.loads(unreal.InspectionEditorLibrary.configure_computer_recovery())
(out/'configure-result.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
assert result.get('success'), result
for path in [widget_path, '/Game/NENS/Environment/Interior/Props/Blueprints/BP_Props_DataComputer']:
    bp = unreal.load_asset(path)
    (out/(path.rsplit('/',1)[-1]+'-after.json')).write_text(unreal.InspectionEditorLibrary.dump_blueprint(bp), encoding='utf-8')
unreal.log('NENS_COMPUTER_RECOVERY_OK')
