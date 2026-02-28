#!/bin/bash

set -e  # stop on error

echo "Creating virtual environment..."
python3 -m venv venv

echo "Activating virtual environment..."
source venv/bin/activate

echo "Upgrading pip..."
pip install --upgrade pip

echo "Installing dependencies..."
pip install -r requirements.txt

#echo "Generating gRPC Python code..."
#python -m grpc_tools.protoc -I../../proto --python_out=. --grpc_python_out=. ../../proto/cardsGame.proto
#cd ../../

echo "Done ✅"