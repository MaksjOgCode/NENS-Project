"""Import prepared ambient loops and set up replaceable ending presentation."""
import json
from pathlib import Path
import unreal

project=Path(unreal.Paths.project_dir())
out=project/'Saved/GamePresentation'
out.mkdir(parents=True,exist_ok=True)
tools=unreal.AssetToolsHelpers.get_asset_tools()
sounds=[]
for name in ['S_GeneralGamePlay_Ambient','S_MainMenu_Ambient']:
    path='/Game/NENS/Audio/Ambience/'+name
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        task=unreal.AssetImportTask()
        for key,value in [('filename',str(project/'SourceArt/Audio/Loops'/(name+'.wav'))),('destination_path','/Game/NENS/Audio/Ambience'),('destination_name',name),('automated',True),('save',False)]:
            task.set_editor_property(key,value)
        tools.import_asset_tasks([task])
    sound=unreal.load_asset(path)
    assert isinstance(sound,unreal.SoundWave)
    sound.set_editor_property('looping',True)
    sound.set_editor_property('sound_group',unreal.SoundGroup.SOUNDGROUP_MUSIC)
    sound.set_editor_property('virtualization_mode',unreal.VirtualizationMode.PLAY_WHEN_SILENT)
    sound.set_editor_property('loading_behavior',unreal.SoundWaveLoadingBehavior.RETAIN_ON_LOAD)
    sound.set_editor_property('compression_quality',80)
    assert unreal.EditorAssetLibrary.save_loaded_asset(sound)
    sounds.append(sound)

end_path='/Game/NENS/UI/Ending/WBP_EndGame'
if not unreal.EditorAssetLibrary.does_asset_exist(end_path):
    factory=unreal.WidgetBlueprintFactory(); factory.set_editor_property('parent_class',unreal.EndGameWidget)
    assert tools.create_asset('WBP_EndGame','/Game/NENS/UI/Ending',unreal.WidgetBlueprint,factory)
instance_path='/Game/NENS/System/BP_NENSGameInstance'
if not unreal.EditorAssetLibrary.does_asset_exist(instance_path):
    factory=unreal.BlueprintFactory(); factory.set_editor_property('parent_class',unreal.GameExperienceInstance)
    assert tools.create_asset('BP_NENSGameInstance','/Game/NENS/System',unreal.Blueprint,factory)
data_path='/Game/NENS/System/DA_GamePresentation'
if not unreal.EditorAssetLibrary.does_asset_exist(data_path):
    factory=unreal.DataAssetFactory(); factory.set_editor_property('data_asset_class',unreal.GamePresentationData)
    assert tools.create_asset('DA_GamePresentation','/Game/NENS/System',unreal.GamePresentationData,factory)
data=unreal.load_asset(data_path)
data.set_editor_property('gameplay_ambient',sounds[0]); data.set_editor_property('main_menu_ambient',sounds[1])
data.set_editor_property('main_menu_map',unreal.load_asset('/Game/NENS/UI/MainMenu/Lvl_MainMenu_test'))
data.set_editor_property('gameplay_map',unreal.load_asset('/Game/NENS/Maps/MainMap/L_MainRooms'))
result=json.loads(unreal.InspectionEditorLibrary.configure_game_presentation())
(out/'configure-result.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
assert result.get('success'),result
for path in [end_path,instance_path,'/Game/NENS/UI/MainMenu/WBP_MainMenu']:
    (out/(path.rsplit('/',1)[-1]+'-after.json')).write_text(unreal.InspectionEditorLibrary.dump_blueprint(unreal.load_asset(path)),encoding='utf-8')
unreal.log('NENS_GAME_PRESENTATION_OK')
