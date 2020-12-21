from os import path

outf = open(f'outCoeffs.txt', "w")

outf.write('{\n')
for j in range(8):
    outf.write('  {\n')
    for a in range(8):
        if path.exists(f'facuIn{j}{a}.txt'):
            infile = open(f'facuIn{j}{a}.txt', "r")
            line1 = infile.readline().split()
            line2 = infile.readline().split()
            line3 = infile.readline().split()
            line4 = infile.readline().split()
            line5 = infile.readline().split()
            line6 = infile.readline().split()
            line7 = infile.readline().split()
            lines = [line1, line2, line3, line4, line5, line6, line7]

            g1 = line4[0]
            g2 = line7[0]
            outf.write('    { \n')
            for k in range(3):
                line1[k] = str(float(line1[k]) * float(g1) * float(g2))

            for k in range(3):
                outf.write('      ')
                for i in range(len(line1)):
                    if i == 1:
                        outf.write('0,\t')
                    if i != 3:
                        outf.write(str(float(lines[k][i]) / 2))
                        outf.write(',\t')
                outf.write('\n')
            outf.write('    },\n')
    outf.write('  },\n')
outf.write('}\n')
outf.close()
