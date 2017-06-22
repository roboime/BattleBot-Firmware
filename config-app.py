#!/usr/bin/python
# -*- coding: utf-8 -*-
import serial
import sys
import array

def always(_):
	return True

cfgs = {
	"kp":           [0, 2, 256.0, 0.0, 256.0, always],
	"ki":           [1, 2, 256.0, 0.0, 256.0, always],
	"kd":           [2, 2, 256.0, 0.0, 256.0, always],
	"pid-blend":    [3, 1, 255.0, 0.0, 1.0, always],
	"enc-frames":   [4, 1, 1.0, 0.0, 32.0, lambda x: int(x) == x],
	"recv-samples": [5, 1, 1.0, 0.0, 31.0, lambda x: int(x) == x and int(x % 2) == 1],
	"right-board":  [6, 1, 1.0, 0.0, 1.0, lambda x: int(x) == x]
}
write_offset = 0x30
ack = 0xac
error_offset = 0xe0

errors = [
	"RAW: Comando inválido!",
	"RAW: Variável inválida!",
	"RAW: Variável de leitura inválida!",
	"RAW: Parâmetros inválidos!",
	"RAW: Buffer muito grande!",
]

def comm(ser, cmd):
	cmd.insert(0, len(cmd))
	ser.write(array.array('B', cmd).tostring())
	req = ser.read()
	if req == '':
		raise serial.SerialException("timeout occured!")
	sz = ord(req)
	return map(ord, ser.read(sz))

def bytestoint(arr, sz):
	arr.reverse()
	num = 0
	for v in arr[:sz]:
		num = (num * 256) + v
	return num
	
def inttobytes(num, sz):
	arr = []
	while sz > 0:
		arr.append(num % 256)
		num = int(num/256)
		sz -= 1
	return arr

if len(sys.argv) < 2:
	print "Uso:", sys.argv[0], "<port>"
	sys.exit(-1)
	
port = sys.argv[1]
baud = 19200

try:
	with serial.Serial(port, baud, timeout=2.0) as ser:
		ser.write('\x55')
		while ser.read() != '\xac':
			ser.write('\x55')
		
		print "RoboIME Sanhaço - plataforma de configuração."
		
		while True:
			cmd = map(lambda x: x.lower(), filter(bool, raw_input("Entre o comando > ").split(' ')))
			
			if len(cmd) >= 2 and cmd[0] == "read":
				if cfgs.has_key(cmd[1]):
					cfg = cfgs[cmd[1]]
					rep = comm(ser, [cfg[0]])
					
					if len(rep) >= 1 + cfg[1] and rep[0] == ack:
						param = bytestoint(rep[1:], cfg[1]) / cfg[2]
						print "Parâmetro", cmd[1], ':', param
					else:
						print "Erro na leitura!"
				else:
					print "Parâmetro inválido!"
			elif len(cmd) >= 3 and cmd[0] == "write":
				if cfgs.has_key(cmd[1]):
					cfg = cfgs[cmd[1]]
					param = float(cmd[2])
					if param >= cfg[3] and param <= cfg[4] and cfg[5](param):
						rep = comm(ser, [write_offset | cfg[0]]
							+ inttobytes(int(param * cfg[2]), cfg[1]))
						if len(rep) >= 1 and rep[0] == ack:
							print "Escrita efetuada com sucesso!"
						else:
							print "Erro na escrita!"
					else:
						print "Parâmetro fora da faixa!"
				else:
					print "Parâmetro inválido!"
			elif len(cmd) >= 1 and cmd[0] == "finish":
				print "Finalizando modo de configuração! Reiniciando uC!"
				comm(ser, [0xff])
				break
			elif len(cmd) >= 1 and cmd[0] == "raw":
				resp = comm(ser, map(lambda x: int(x, 16), cmd[1:]))
				line = ' '.join(map(lambda x: format(x, 'x'), resp))
				print "Resposta ao comando puro:", line
			else:
				print "Comando inválido!"
except serial.SerialException as e:
	print "An error occured:", e
	sys.exit(-1)


