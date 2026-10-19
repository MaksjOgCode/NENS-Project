"""One-time presentation update. Keeps the original board and all puzzle Blueprint graphs.

Run using UnrealEditor-Cmd -run=pythonscript -script=<absolute path> -NullRHI
with the editor closed. Already-styled layouts are left alone on subsequent runs.
"""
import copy
import json
from pathlib import Path
import unreal

out = Path(unreal.Paths.project_dir()) / 'Saved' / 'WiresPresentation'
out.mkdir(parents=True, exist_ok=True)
bp = unreal.load_asset('/Game/Minigames/Wires/WBP_WiresMiniGame')
before = json.loads(unreal.InspectionEditorLibrary.dump_blueprint(bp))
if not (out / 'before-widget.json').exists():
    (out / 'before-widget.json').write_text(json.dumps(before, indent=2), encoding='utf-8')
result = json.loads(unreal.InspectionEditorLibrary.style_wires_minigame())
(out / 'setup-result.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
assert result.get('success'), result
after = json.loads(unreal.InspectionEditorLibrary.dump_blueprint(bp))
(out / 'after-widget.json').write_text(json.dumps(after, indent=2), encoding='utf-8')
validation = json.loads(unreal.InspectionEditorLibrary.compile_and_save(bp, False))
(out / 'compile-result.json').write_text(json.dumps(validation, indent=2), encoding='utf-8')

def stable_graphs(data):
    graphs = copy.deepcopy(data['graphs'])
    for graph in graphs:
        for node in graph['nodes']:
            for pin in node['pins']:
                if pin['name'] == 'ErrorTolerance' and pin['hidden'] and not pin['links']:
                    pin.pop('id', None)  # Promotable integer operators regenerate this unused pin.
    return graphs

def board_widgets(data):
    widgets = {w['name']: copy.deepcopy(w) for w in data['widgets']}
    names = {'Board'}
    while True:
        descendants = {w['name'] for w in data['widgets'] if w.get('parent') in names}
        if descendants <= names:
            break
        names |= descendants
    # The only change inside the board is the background's opaque-padding crop.
    background = widgets['Img_Bg']
    background['properties']['Brush'] = background['properties']['Brush'].split(',UVRegion=')[0]
    background['slot']['properties'].pop('LayoutData')
    return {name: widgets[name] for name in sorted(names)}

checks = {
    'puzzle_graphs_preserved': stable_graphs(before) == stable_graphs(after),
    'board_preserved_except_background_padding_crop': board_widgets(before) == board_widgets(after),
    'presentation_parent': after['parent_class'] == '/Script/NENS_Project.WiresMinigameWidget',
    'compile_without_warnings': validation.get('success') and validation.get('warnings') == 0,
}
(out / 'invariants.json').write_text(json.dumps(checks, indent=2), encoding='utf-8')
assert all(checks.values()), checks
unreal.log('NENS_WIRES_PRESENTATION_OK ' + json.dumps(checks))
