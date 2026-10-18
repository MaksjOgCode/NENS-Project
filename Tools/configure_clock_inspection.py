"""Import clock art, configure editable puzzle data, and connect the existing clock actor.

Uses the shared inspection session and pickup system. Existing data is not reset.
"""
import json
import runpy
from pathlib import Path
import unreal

out = Path(unreal.Paths.project_dir()) / 'Saved' / 'ClockPuzzle'
out.mkdir(parents=True, exist_ok=True)
tools = unreal.AssetToolsHelpers.get_asset_tools()
folder = '/Game/NENS/UI/Inspection'
source = Path('C:/Users/laqks/OneDrive/Desktop/character')

def texture(name):
    path = folder + '/Textures/' + name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.load_asset(path)
    task = unreal.AssetImportTask()
    for prop, value in [('filename', str(source / (name + '.png'))), ('destination_path', folder + '/Textures'), ('destination_name', name), ('automated', True), ('save', False)]:
        task.set_editor_property(prop, value)
    tools.import_asset_tasks([task])
    asset = unreal.load_asset(path)
    assert isinstance(asset, unreal.Texture2D), path
    asset.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_UI)
    asset.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    asset.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset)
    return asset

def data_asset(name, cls):
    path = folder + '/' + name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        asset = unreal.load_asset(path)
        assert isinstance(asset, cls), path
        return asset, False
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', cls)
    asset = tools.create_asset(name, folder, cls, factory)
    assert asset, path
    return asset, True

closed = texture('T_Inspection_Furniture_FloorClock_0')
opened = texture('T_Inspection_Furniture_FloorClock_1')
short = texture('T_Inspection_FloorClock_ShortArrow')
long = texture('T_Inspection_FloorClock_LongArrow')
card = unreal.load_asset(folder + '/Textures/T_PunchCard')
assert card
scene, created = data_asset('DA_Inspection_FloorClock', unreal.InspectionSceneData)
if created:
    scene.set_editor_property('background', opened)
    scene.set_editor_property('canvas_size', unreal.Vector2D(1875, 1902))
    pickup = unreal.InspectionPickup()
    for prop, value in [('pickup_id','FloorClock_PunchCard'), ('item_id','PunchCard'), ('image',card), ('position',unreal.Vector2D(720,1070)), ('size',unreal.Vector2D(430,258))]:
        pickup.set_editor_property(prop, value)
    scene.set_editor_property('pickups', [pickup])
    assert unreal.EditorAssetLibrary.save_loaded_asset(scene)

puzzle, created = data_asset('DA_ClockPuzzle_FloorClock', unreal.ClockPuzzleData)
puzzle_created = created
if created:
    puzzle.set_editor_property('closed_background', closed)
    puzzle.set_editor_property('canvas_size', unreal.Vector2D(1875,1902))
    puzzle.set_editor_property('dial_center', unreal.Vector2D(938,479))
    puzzle.set_editor_property('dial_radius', 118)
    for prop, tex, uv0, uv1, pivot, size, native, length, tray, initial, target in [
        ('short_hand',short,(.364,.165),(.487,.498),(61/123,305/333),(29.52,79.92),0,73.2,(350,760),210,0),
        ('long_hand',long,(.341,.452),(.731,.594),(27/390,27/142),(101.4,36.92),108,98.7,(1420,760),300,108),
    ]:
        art = unreal.ClockHandArt()
        for name, value in [('texture',tex),('uv_min',unreal.Vector2D(*uv0)),('uv_max',unreal.Vector2D(*uv1)),('pivot',unreal.Vector2D(*pivot)),('size',unreal.Vector2D(*size)),('native_angle',native),('tip_length',length),('tray_position',unreal.Vector2D(*tray)),('initial_angle',initial),('target_angle',target)]:
            art.set_editor_property(name, value)
        puzzle.set_editor_property(prop, art)
    assert unreal.EditorAssetLibrary.save_loaded_asset(puzzle)

if puzzle_created:
    runpy.run_path(str(Path(__file__).with_name('style_clock_inspection.py')))

widget_path = folder + '/WBP_ClockInspection'
if not unreal.EditorAssetLibrary.does_asset_exist(widget_path):
    assert tools.duplicate_asset('WBP_ClockInspection', folder, unreal.load_asset(folder + '/WBP_InspectionScene'))
result = json.loads(unreal.InspectionEditorLibrary.configure_clock_inspection())
(out / 'setup-result.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
assert result.get('success'), result
for name, path in [('BP_FloorClock','/Game/NENS/Environment/Interior/Blueprints/BP_FloorClock'), ('WBP_ClockInspection',widget_path)]:
    asset = unreal.load_asset(path)
    validation = json.loads(unreal.InspectionEditorLibrary.compile_and_save(asset, False))
    assert validation['success'] and not validation['warnings'], validation
    (out / (name + '-configured.json')).write_text(unreal.InspectionEditorLibrary.dump_blueprint(asset), encoding='utf-8')
unreal.log('NENS_CLOCK_SETUP_OK')
