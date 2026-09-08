#!/usr/bin/env python3
"""Integration checks against a running davtools server; requires Pillow."""
import io
import http.client
import urllib.parse
import sys
import urllib.request
import urllib.error
import xml.etree.ElementTree as ET
from PIL import Image

BASE = sys.argv[1].rstrip('/') if len(sys.argv) > 1 else 'http://127.0.0.1:8081'
def request(path, method='GET', data=None):
    try:
        with urllib.request.urlopen(urllib.request.Request(BASE+path, data=data, method=method,
                headers={'Depth': '1'}), timeout=60) as r:
            return r.status, r.read(), r.headers
    except urllib.error.HTTPError as e:
        return e.code, e.read(), e.headers

def png(path):
    status, data, headers = request(path)
    assert status == 200 and headers['Content-Type'].startswith('image/png'), (path, status, data[:100])
    image = Image.open(io.BytesIO(data)).convert('RGB')
    assert image.size == (1600, 1287), image.size
    return image, data

def choose(op, field, value):
    assert request(f'/convert/{op}/settings/{field}/{value}.png', 'DELETE')[0] == 204

# Reject an oversized Content-Length before any large body needs to be sent.
parsed=urllib.parse.urlsplit(BASE)
if parsed.hostname in ('127.0.0.1', 'localhost', '::1'):
    connection_type=http.client.HTTPSConnection if parsed.scheme == 'https' else http.client.HTTPConnection
    connection=connection_type(parsed.netloc,timeout=30)
    connection.putrequest('PUT','/convert/halftone/in/too-large.png')
    connection.putheader('Content-Length',str(50*1024*1024+1))
    connection.endheaders()
    assert connection.getresponse().status == 413
    connection.close()
    print('PASS oversized upload returns HTTP 413')
else:
    print('SKIP header-only upload limit probe through proxy; covered on direct server')

try:
    for op, fields in [('threshold', {'value':100}), ('halftone', {'density':100, 'size':63}), ('bayer', {'grid':4}), ('dither', {'method':2, 'tone':100, 'grain':8})]:
        root=f'/convert/{op}/settings/'
        source, source_bytes=png(root+'source.png')
        before=request(root+'current.png')[1]
        for field, count in fields.items():
            status, xml, _=request(root+field+'/', 'PROPFIND')
            assert status == 207
            doc=ET.fromstring(xml)
            files=[r for r in doc.findall('{DAV:}response') if r.find('{DAV:}href').text.endswith('.png')]
            assert len(files)==count, (op, field, len(files))
            leaf=files[0].find('{DAV:}href').text
            png(leaf)
            assert request(leaf, 'PROPFIND')[0] == 207
            assert request(leaf, 'HEAD')[2]['Content-Length'] == str(len(request(leaf)[1]))
        assert request(root+'current.png')[1] == before, 'Preview reads changed settings'
        image, data=png(root+'current.png')
        colors = set(image.convert('L').tobytes())
        if op == 'halftone':
            assert len(colors) > 2, 'Missing smooth dot edges'
        else:
            assert colors <= {0,255}, op
        assert request(root+'source.png', 'DELETE')[0] == 403
        assert request(root+'invalid/1.png', 'DELETE')[0] == 400
        assert request(root+'source.png', 'HEAD')[2]['Content-Length'] == str(len(source_bytes))
        assert request(f'/convert/{op}/in/preview-test.png','PUT',source_bytes)[0] == 201
        converted=Image.open(io.BytesIO(request(f'/convert/{op}/out/preview-test_{op}.png')[1])).convert('RGB')
        assert converted.size == image.size and converted.tobytes() == image.tobytes(), 'Preview and conversion pixels differ'
        for section in ['in','out']:
            name='preview-test.png' if section=='in' else f'preview-test_{op}.png'
            request(f'/convert/{op}/{section}/{name}', 'DELETE')
        print('PASS', op, 'listings, previews, HEAD, read-only source and actual conversion')
    means=[]
    for density in [20,50,80]:
        choose('halftone','density',density)
        image,_=png('/convert/halftone/settings/current.png')
        means.append(sum(image.convert('L').tobytes())/(1600*1287))
    assert means[0] > means[1] > means[2], means
    choose('halftone','size',4)
    small=request('/convert/halftone/settings/current.png')[1]
    choose('halftone','size',16)
    assert request('/convert/halftone/settings/current.png')[1] != small
    outputs=[]
    for grid in [2,4,8,16]:
        choose('bayer','grid',grid)
        outputs.append(request('/convert/bayer/settings/current.png')[1])
    assert len(set(outputs))==4
    assert request('/convert/bayer/settings/grid/3.png','DELETE')[0] == 400
    print('PASS halftone density/size and all four Bayer grids affect output')
    before=request('/convert/dither/settings/current.png')[1]
    for parameter, value in [('method',2), ('tone',80), ('grain',4)]:
        preview=request(f'/convert/dither/settings/current.png?{parameter}={value}')[1]
        assert preview != before, (parameter, 'preview parameter ignored')
        assert request('/convert/dither/settings/current.png')[1] == before, 'Preview mutated settings'
        choose('dither', parameter, value)
        assert request('/convert/dither/settings/current.png')[1] == preview, 'Setting not applied'
        choose('dither',parameter, {'method':1, 'tone':50, 'grain':1}[parameter])
    print('PASS dither algorithm, tone, grain and non-mutating explicit previews')
    # A uniform mid-gray field must produce genuinely round, symmetric dots.
    fixture=Image.new('RGB',(96,96),(188,188,188))
    buffer=io.BytesIO(); fixture.save(buffer, format='PNG')
    choose('halftone','size',24); choose('halftone','density',50)
    assert request('/convert/halftone/in/dot-geometry.png','PUT',buffer.getvalue())[0] == 201
    data=request('/convert/halftone/out/dot-geometry_halftone.png')[1]
    result=Image.open(io.BytesIO(data)).convert('L')
    dot=result.crop((24,24,48,48))
    assert dot.tobytes() == dot.transpose(Image.Transpose.FLIP_LEFT_RIGHT).tobytes()
    assert dot.tobytes() == dot.transpose(Image.Transpose.FLIP_TOP_BOTTOM).tobytes()
    assert dot.getpixel((12,12)) == 0 and dot.getpixel((0,0)) == 255
    assert any(0 < p < 255 for p in dot.tobytes()), 'No antialiasing'
    request('/convert/halftone/in/dot-geometry.png','DELETE')
    request('/convert/halftone/out/dot-geometry_halftone.png','DELETE')
    print('PASS circular, symmetric, antialiased halftone dots')
finally:
    choose('threshold','value',50)
    choose('halftone','density',50)
    choose('halftone','size',24)
    choose('bayer','grid',4)
    choose('dither','method',1)
    choose('dither','tone',50)
    choose('dither','grain',1)
