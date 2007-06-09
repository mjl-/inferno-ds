#!/bin/sh

5a -t rom.s  && 5a -t div.s && tc test.c
5l -o rom.gba rom.5 
5l  -o rom.gba rom.t div.t test.t 
open rom.gba
tc  test.c
