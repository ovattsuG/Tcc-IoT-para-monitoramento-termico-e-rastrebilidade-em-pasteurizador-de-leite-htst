@echo off
rem Botao direito, Executar como administrador. Chama o instalar.ps1 da mesma pasta (pendrive).
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0instalar.ps1"
pause
