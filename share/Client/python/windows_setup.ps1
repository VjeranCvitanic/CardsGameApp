Write-Host "Creating virtual environment..."
python -m venv venv

Write-Host "Activating virtual environment..."
.\venv\Scripts\Activate.ps1

Write-Host "Upgrading pip..."
python -m pip install --upgrade pip

Write-Host "Installing dependencies..."
pip install -r requirements.txt

Write-Host "Generating gRPC Python code..."

# Ensure output directory exists
if (!(Test-Path "out")) {
    New-Item -ItemType Directory -Path "out"
}

# Generate protobuf + gRPC code
#python -m grpc_tools.protoc `
#    -I../../proto `
#    --python_out=. `
#    --grpc_python_out=. `
#    ../../proto/cardsGame.proto

Write-Host "Done ✅"