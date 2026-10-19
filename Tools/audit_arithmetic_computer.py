"""Read-only check of saved actor instances and the four imported frames."""
import json
from pathlib import Path
import unreal

out = Path(unreal.Paths.project_saved_dir())/'ArithmeticPuzzle'
snapshot = json.loads(unreal.InspectionEditorLibrary.dump_level('/Game/NENS/Maps/MainMap/L_MainRooms'))
expected = {
    'BP_ComputerCabinet_C': ('Arithmetic','WBP_ArithmeticComputer_C'),
    'BP_Props_ElectroStation_C': ('Power','WBP_WiresMiniGame_C'),
    'BP_Props_DataComputer_C': (None,'WBP_ComputerRecovery_C'),
}
report = []
for item in snapshot['actors']:
    name = item['class'].rsplit('.',1)[-1]
    if name not in expected: continue
    actor = unreal.find_object(None,item['path'])
    assert actor, item['path']
    system,widget = expected[name]
    component = actor.get_component_by_class(unreal.MinigameComponent if system else unreal.ComputerRecoveryComponent)
    assert component, item['path']
    assigned = component.get_editor_property('widget_class')
    assert assigned and assigned.get_name()==widget, (item['path'],str(assigned))
    if system:
        actual = component.get_editor_property('repaired_system')
        assert actual == getattr(unreal.ComputerSystem,system.upper()), (item['path'],str(actual),system)
    sprite = actor.get_component_by_class(unreal.PaperSpriteComponent)
    report.append(dict(actor=item['path'],widget=assigned.get_path_name(),system=system,
                       sprite=sprite.get_sprite().get_path_name(),
                       relative_location=str(sprite.get_editor_property('relative_location')),
                       relative_rotation=str(sprite.get_editor_property('relative_rotation')),
                       relative_scale=str(sprite.get_editor_property('relative_scale3d'))))
assert {item['actor'].split(':')[-1].rsplit('.',1)[-1].split('_C_')[0] for item in report} >= {
    'BP_ComputerCabinet','BP_Props_ElectroStation','BP_Props_DataComputer'}, report
data = unreal.load_asset('/Game/NENS/UI/Arithmetic/DA_ArithmeticComputer')
assert data.answer==29 and len(data.frames)==4
assert [frame.get_name() for frame in data.frames]==[f'T_Inspection_Furniture_Computer_{i}' for i in range(4)]
(out/'saved-instance-audit.json').write_text(json.dumps(dict(actor_count=len(snapshot['actors']),instances=report),indent=2),encoding='utf-8')
unreal.log('NENS_ARITHMETIC_SAVED_INSTANCES_OK')
