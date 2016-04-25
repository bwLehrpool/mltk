@echo off

setlocal EnableDelayedExpansion

rem Kein sleep oder timeout bis Win7, also einen
rem haesslichen Hack (Zeitverz. ca. 5 sec)
rem Sinn der Verzoegerung: Mitunter (1/3) wird
rem sonst das erste Netzlaufwerk nicht eingehaengt

@ping -n 5 localhost> nul

rem Konfigdatei auslesen und erstmal in Feld:
set n=0
for /f %%i in (b:\sharelw.cfg) do (
    set array[!n!]=%%i
    set /A n+=1
)

rem Aufruf für net use erzeugen:
set "i=0"
for %%b in (K L M N O P Q R S T U V W X Y Z) do (
    if defined array[!i!] (
       call set aufruf[!i!]=%%b: %%array[!i!]%%
       set /A "i+=1"
    ) else (
    goto :Schleifenende
    )
)
:Schleifenende

rem net use-Aufrufe
set "i=0"
:Ausdrucken
if defined aufruf[!i!] (
    call net use %%aufruf[!i!]%%
    set /a "i+=1"
    GOTO :Ausdrucken
)

exit /B
