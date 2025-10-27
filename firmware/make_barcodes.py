
from io import BytesIO

#from barcode import Code39
#from barcode.writer import ImageWriter
import qrcode
from PIL import Image, ImageDraw, ImageFont

data = [
        *[(f'SP-PITCH-V2-{i:04d}', f'{i:04d}') for i in range(1, 101)],
        *[(f'SP-ALT-A-V1-{i:04d}', f'{i:04d}') for i in range(101, 106)],
        *[(f'SP-ALT-B-V1-{i:04d}', f'{i:04d}') for i in range(106, 111)],
        ]

leader = 20
font = ImageFont.truetype('Roboto-Bold.ttf', size=18)
img = Image.new('1', (len(data)*180 + leader, 52), 'white')

#io = BytesIO()
#Code39('A2000', writer=ImageWriter()).write(io)
#barcode_img = Image.open(io)
#img.paste(barcode_img.resize((barcode_img.size[0]//2, barcode_img.size[1]), Image.NEAREST), (0, -15))

for i, (qr_data, serial) in enumerate(data):
    offx = i*180 + leader
    qr = qrcode.QRCode(version=1, error_correction=qrcode.constants.ERROR_CORRECT_Q, box_size=2, border=0)
    qr.add_data(qr_data)
    qr_img = qr.make_image()
    for i in range(2):
        img.paste(qr_img.convert('1'), (offx + 15 + i*42 + i*15, 5))

    draw = ImageDraw.Draw(img)
    draw.text((offx + 120, 4), 'Serial', font=font, fill='black')
    draw.text((offx + 120, 27), serial, font=font, fill='black')
    draw.line((offx + 179, 0, offx + 179, 52), fill='black', width=1)
img.save('/tmp/test.png')
