package forge;

import sys.FileSystem;
import sys.io.File;
import haxe.crypto.Crc32;

function isTargetOutOfDate( src_path : String, target_path : String ) {
    if (!FileSystem.exists(src_path)) return false;
    if (!FileSystem.exists(target_path))return true;

    var dest_stat = FileSystem.stat(target_path);
    var src_stat = FileSystem.stat(src_path);

    return src_stat.mtime.getTime() > dest_stat.mtime.getTime();
}

function overwriteIfDifferentString( path : String, content : String ) : Bool {
    if (FileSystem.exists(path)) {
        var fileBytes = File.getBytes( path );
        var contentBytes = haxe.io.Bytes.ofString(content);

        if (fileBytes.length == contentBytes.length) {
            var fileCrc = Crc32.make( fileBytes );
            var stringCrc = Crc32.make( contentBytes );

            if (fileCrc == stringCrc) {
//                trace('The contents match, skipping ${path}');
                return false;
            }
  //          trace('The contents have the same length, but differ somewhere, CRC ${fileCrc} vs ${stringCrc}, overwriting contents');
        }
    //    trace('Path ${path} has a different length, ${fileBytes.length} vs ${contentBytes.length}, overwriting contents');
    }else {
      //  trace('Path ${path} doesn\'t exist, saving contents');

    }

    File.saveBytes(path, haxe.io.Bytes.ofString(content));
    return true;
}

function overwriteIfDifferentBytes( path : String, contentBytes : haxe.io.Bytes ) : Bool {
    if (FileSystem.exists(path)) {
        var fileBytes = File.getBytes( path );

        if (fileBytes.length == contentBytes.length) {
            var fileCrc = Crc32.make( fileBytes );
            var stringCrc = Crc32.make( contentBytes );

            if (fileCrc == stringCrc) {
                return false;
            }
        }
    }

    File.saveBytes(path, contentBytes);
    return true;
}