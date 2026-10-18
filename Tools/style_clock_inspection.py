"""Apply the clock's English UI font and hand presentation without resetting puzzle progress settings.

Run with UnrealEditor-Cmd -ExecutePythonScript=<absolute path> -RenderOffscreen.
Font import needs an initialized Slate application, so use the editor rather than a commandlet.
Only the clock puzzle data and the new font resources are saved.
"""
import json
from pathlib import Path
import unreal

project = Path(unreal.Paths.project_dir())
folder = '/Game/NENS/UI/Fonts'
font_path = folder + '/F_IMFellEnglish'
tools = unreal.AssetToolsHelpers.get_asset_tools()

if not unreal.EditorAssetLibrary.does_asset_exist(font_path):
    factory = unreal.FontFileImportFactory()
    factory.set_editor_property('batch_create_font_asset', unreal.BatchCreateFontAsset.YES)
    task = unreal.AssetImportTask()
    for prop, value in [
        ('filename', str(project / 'Content/NENS/UI/Fonts/Source/IMFellEnglish/IMFeENrm28P.ttf')),
        ('destination_path', folder), ('destination_name', 'FF_IMFellEnglish'),
        ('automated', True), ('save', True), ('factory', factory),
    ]:
        task.set_editor_property(prop, value)
    tools.import_asset_tasks([task])
    fonts = [unreal.load_asset(path) for path in unreal.EditorAssetLibrary.list_assets(folder, recursive=False)]
    fonts = [font for font in fonts if isinstance(font, unreal.Font)]
    assert len(fonts) == 1, [font.get_path_name() for font in fonts]
    if fonts[0].get_path_name().split('.')[0] != font_path:
        assert unreal.EditorAssetLibrary.rename_asset(fonts[0].get_path_name(), font_path)

font = unreal.load_asset(font_path)
assert isinstance(font, unreal.Font)
puzzle = unreal.load_asset('/Game/NENS/UI/Inspection/DA_ClockPuzzle_FloorClock')
assert isinstance(puzzle, unreal.ClockPuzzleData)
puzzle.set_editor_property('interface_font', font)
puzzle.set_editor_property('tray_scale', 2.5)
for name, tip, position, original_tip, original_size in [
    ('short_hand', 54.0, (260, 740), 73.2, (29.52, 79.92)),
    ('long_hand', 104.0, (1615, 740), 98.7, (101.4, 36.92)),
]:
    art = puzzle.get_editor_property(name)
    scale = tip / original_tip
    art.set_editor_property('size', unreal.Vector2D(original_size[0] * scale, original_size[1] * scale))
    art.set_editor_property('tip_length', tip)
    art.set_editor_property('tray_position', unreal.Vector2D(*position))
    puzzle.set_editor_property(name, art)
assert unreal.EditorAssetLibrary.save_loaded_asset(puzzle)

result = {'font': font.get_path_name(), 'short_tip': 54, 'long_tip': 104, 'tray_scale': 2.5}
(project / 'Saved/ClockPuzzle/ui-style-result.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
unreal.log('NENS_CLOCK_STYLE_OK ' + json.dumps(result))
