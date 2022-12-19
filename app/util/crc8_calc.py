import sys, getopt
from crc import Calculator, Crc8

calculator = Calculator(Crc8.CCITT)

def main(argv):
	inputfile = ''
	try:
		opts, args = getopt.getopt(argv,"hi:",["ifile="])
	except getopt.GetoptError:
		print ('crc8_calc.py -i <inputfile>')
		sys.exit(2)
	for opt, arg in opts:
		if opt == '-h':
			print ('crc8_calc.py -i <inputfile>')
			sys.exit()
		elif opt in ("-i", "--ifile"):
			inputfile = arg
	if not inputfile:
		print ('crc8_calc.py -i <inputfile>')
		sys.exit(2)

	with open(inputfile, "rb") as f:
		bytes = f.read()
		print(calculator.checksum(bytes))

if __name__ == "__main__":
	main(sys.argv[1:])
