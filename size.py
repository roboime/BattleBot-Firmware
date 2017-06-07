#!/usr/bin/python
import subprocess

string = subprocess.check_output(['avr-size', 'out.elf'])
tbl = map(lambda ls: map(lambda s: s.strip(), filter(bool, ls.split('\t'))),
	filter(bool, string.split('\n')))

val = {}
for i in range(len(tbl[0])):
	val[tbl[0][i]] = tbl[1][i]
	
total_program = int(val['data']) + int(val['text'])
total_data = int(val['data']) + int(val['bss'])

print "Total bytes written to flash:", total_program
print "Total bytes used on SRAM:", total_data

