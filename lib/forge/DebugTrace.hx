package forge;

#if macro
import haxe.macro.Expr;
#end

class DebugTrace {
    public static macro function trace(e : Expr) : Expr {
        return macro {};
//        return macro trace($e);
    }
}