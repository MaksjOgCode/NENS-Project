"""Import the arithmetic inspection, move wires to the station, connect the map.

Run with the editor closed using UnrealEditor-Cmd -run=pythonscript.
Only the two matching decorative props are replaced in L_MainRooms.
"""
import json
from pathlib import Path
import unreal

project = Path(unreal.Paths.project_dir())
out = project/'Saved/ArithmeticPuzzle'
out.mkdir(parents=True, exist_ok=True)
tools = unreal.AssetToolsHelpers.get_asset_tools()
folder = '/Game/NENS/UI/Arithmetic'
frames = []
for index in range(4):
    name = f'T_Inspection_Furniture_Computer_{index}'
    path = folder+'/Textures/'+name
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        task = unreal.AssetImportTask()
        for key, value in [('filename', str(project/'SourceArt/Inspection/Arithmetic'/(name+'.png'))),
                           ('destination_path',folder+'/Textures'),('destination_name',name),
                           ('automated',True),('save',False),('replace_existing',False)]:
            task.set_editor_property(key, value)
        tools.import_asset_tasks([task])
    texture = unreal.load_asset(path)
    assert isinstance(texture, unreal.Texture2D), path
    for key, value in [('compression_settings',unreal.TextureCompressionSettings.TC_DEFAULT),
                       ('compression_no_alpha',False),('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI),
                       ('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS),
                       ('filter',unreal.TextureFilter.TF_BILINEAR),('srgb',True),('max_texture_size',2048),
                       ('address_x',unreal.TextureAddress.TA_CLAMP),('address_y',unreal.TextureAddress.TA_CLAMP)]:
        texture.set_editor_property(key,value)
    assert unreal.EditorAssetLibrary.save_loaded_asset(texture)
    frames.append(texture)

data_path = folder+'/DA_ArithmeticComputer'
if not unreal.EditorAssetLibrary.does_asset_exist(data_path):
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class',unreal.ArithmeticPuzzleData)
    assert tools.create_asset('DA_ArithmeticComputer',folder,unreal.ArithmeticPuzzleData,factory)
data = unreal.load_asset(data_path)
data.set_editor_property('frames',frames)
data.set_editor_property('answer',29)
assert unreal.EditorAssetLibrary.save_loaded_asset(data)
widget_path = folder+'/WBP_ArithmeticComputer'
if not unreal.EditorAssetLibrary.does_asset_exist(widget_path):
    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property('parent_class',unreal.ArithmeticMinigameWidget)
    assert tools.create_asset('WBP_ArithmeticComputer',folder,unreal.WidgetBlueprint,factory)
computer_path = '/Game/NENS/Environment/Interior/Blueprints/BP_ComputerCabinet'
station_path = '/Game/NENS/Environment/Interior/Props/Blueprints/BP_Props_ElectroStation'
if not unreal.EditorAssetLibrary.does_asset_exist(station_path):
    assert unreal.EditorAssetLibrary.duplicate_asset(computer_path,station_path)
result = json.loads(unreal.InspectionEditorLibrary.configure_arithmetic_minigame())
(out/'configure-result.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
assert result.get('success'), result
for path in [widget_path,computer_path,station_path,'/Game/NENS/UI/Recovery/WBP_ComputerRecovery']:
    (out/(path.rsplit('/',1)[-1]+'.json')).write_text(
        unreal.InspectionEditorLibrary.dump_blueprint(unreal.load_asset(path)),encoding='utf-8')
if not (out/'map-before.json').exists():
    (out/'map-before.json').write_text(unreal.InspectionEditorLibrary.dump_level('/Game/NENS/Maps/MainMap/L_MainRooms'),encoding='utf-8')
result = json.loads(unreal.InspectionEditorLibrary.connect_arithmetic_level('/Game/NENS/Maps/MainMap/L_MainRooms'))
(out/'map-result.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
assert result.get('success'), result
(out/'map-after.json').write_text(unreal.InspectionEditorLibrary.dump_level('/Game/NENS/Maps/MainMap/L_MainRooms'),encoding='utf-8')
collection = project/'Content/Collections/NENS_Demo_Props_Ready.collection'
text = collection.read_text(encoding='utf-8')
for path in [station_path,computer_path]:
    entry = path+'.'+path.rsplit('/',1)[-1]
    if entry not in text: text += entry+'\n'
collection.write_text(text,encoding='utf-8')
unreal.log('NENS_ARITHMETIC_CONFIGURE_OK')
