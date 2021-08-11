#!/usr/bin/env python3

from PIL import Image

import argparse
import sys

gif_width = 2048
font_height = 8
skip_stripes = 1
stripes = 4

def bin2header(data, var_name='var'):
	out = []
	out.append('static constexpr uint8_t {var_name}[] = {{'.format(var_name=var_name))
	l = [ data[i:i+12] for i in range(0, len(data), 12) ]
	for i, x in enumerate(l):
		line = ', '.join([ '0x{val:02x}'.format(val=c) for c in x ])
		out.append('  {line}{end_comma}'.format(line=line, end_comma=',' if i<len(l)-1 else ''))
	out.append('};')
	out.append('static constexpr size_t {var_name}_len = {data_len};'.format(var_name=var_name, data_len=len(data)))
	return '\n'.join(out)

def main():

	parser = argparse.ArgumentParser(description='Generate font header')
	parser.add_argument('-i', '--input', required=True , help='Input file')
	parser.add_argument('-o', '--out', required=True , help='Output file')
	parser.add_argument('-v', '--var', required=True , help='Variable name')

	args = parser.parse_args()
	if not args:
		return 1

	with Image.open(args.input) as source:
		source.seek(1)
		source = source.convert("1")
	
		outputdata = Image.new("1", (font_height, gif_width * stripes))
		for s in range(stripes):
			outputdata.paste(source.crop((0, (s + skip_stripes) * font_height, gif_width, (s + skip_stripes + 1) * font_height)).transpose(Image.ROTATE_270), (0, s * gif_width))

		bytes = outputdata.tobytes()

		with open(args.out, 'w') as outputfile:
			outputfile.write(bin2header(bytes, args.var))

if __name__ == '__main__':
	sys.exit(main())
