#!/usr/bin/env python3
"""Integration checks against a running davtools server; requires Pillow."""
import io
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
    assert image.size == (365, 274), image.size
    return image, data

def choose(op, field, value):
    assert request(f'/convert/{op}/settings/{field}/{value}.png', 'DELETE')[0] == 204

try:
    for op, fields in [('threshold', {'value':100}), ('halftone', {'density':100, 'size':31}), ('bayer', {'grid':4}), ('dither', {})]:
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
        assert set(image.getdata()) <= {(0,0,0), (255,255,255)}, op
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
        means.append(sum(image.convert('L').getdata())/(365*274))
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
finally:
    choose('threshold','value',50)
    choose('halftone','density',50)
    choose('halftone','size',8)
    choose('bayer','grid',4)
