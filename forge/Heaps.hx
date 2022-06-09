package forge;

import hl.Bytes;
import mirage.Native;
import h3d.col.Point;
import h3d.prim.Polygon;

function indexedMeshToPrimitive(idtm : IndexedTriMesh) {

    var mesh = idtm;
    var vc = mesh.vertexCount();

//    trace('Mesh has ${vc} verts and ${mesh.triangleCount()} tris');

    var pos = new Array<Single>();
    pos.resize(vc * 3);
    
    var indices = new Array<Int>();
    indices.resize(mesh.triangleCount() * 3 );
    

    mesh.getPositions(Bytes.getArray(pos));
    mesh.getIndices(Bytes.getArray(indices));

    var hPoints = new Array<Point>();

    hPoints.resize(vc);

    for (i in 0...vc){
        hPoints[i] = new Point( pos[i * 3 + 0], pos[i * 3 + 1], pos[i * 3 + 2]);
    }
    var hIndices = new hxd.IndexBuffer();
    var ic = indices.length;
    for (i in 0...ic) {
        hIndices.push(indices[i]);
    }
    var pg = new Polygon(hPoints, hIndices);

    return pg;
}