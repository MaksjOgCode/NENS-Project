"""Adapt the existing wires widget and connect BP_ComputerCabinet via Unreal API."""
import json
from pathlib import Path
import unreal

out = Path(unreal.Paths.project_dir()) / 'Saved' / 'WiresIntegration'
out.mkdir(parents=True, exist_ok=True)
result = json.loads(unreal.InspectionEditorLibrary.configure_wires_minigame())
(out / 'setup-result.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
assert result.get('success'), result
unreal.log('NENS_WIRES_SETUP_OK ' + json.dumps(result))
