from random import randint

for n in range(10000):
	I0 = randint(0, 2**31)
	I1 = (65539 * I0) % 2**31
	I2 = (65539 * I1) % 2**31
	
	I0 = float(I0) / (2**31)	
	I1 = float(I1) / (2**31)
	I2 = float(I2) / (2**31)	
	print str(I0) + " " + str(I1) + " " + str(I2)  
