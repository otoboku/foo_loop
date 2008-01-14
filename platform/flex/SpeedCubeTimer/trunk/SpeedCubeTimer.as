import flash.utils.Timer;
import flash.events.TimerEvent;
import flash.events.FocusEvent;

import mx.controls.Alert;

private const MIN_MASK:String = "00";
private const SEC_MASK:String = "00";
private const MS_MASK:String  = "000";

private const TIMER_INTERVAL:int = 10;

private var baseTimer:int;
private var t:Timer;
private var result:Array;

private const CUBE_SIZE:int = 3;
private const SEQ_LEN:int = 30;

private function init():void {
    counter.text = "00:00.000";

    t = new Timer(TIMER_INTERVAL);
    t.addEventListener(TimerEvent.TIMER, function(e:TimerEvent):void {
            var d:Date = new Date(getTimer() - baseTimer);
            var min:String = (MIN_MASK + d.minutes).substr(-MIN_MASK.length);
            var sec:String = (SEC_MASK + d.seconds).substr(-SEC_MASK.length);
            var ms:String  = (MS_MASK + d.milliseconds).substr(-MS_MASK.length);
            counter.text = min + ":" + sec + "." + ms;
        });
}

private function toggleTimer():void {
    if ( t.running ) {
        t.stop();
    }
    else {
        baseTimer = getTimer();
        t.start();
    }
}

private function resetTimer():void {
    t.reset();
    counter.text = "00:00.000";
    scramble.text = "";
}

// private function showResult():void {
// }

private function showAbout():void {
    var messages:Array =
        [
         "Start & Stop を押すとタイマーがスタート/ストップします。当たり前ですが。",
         "Reset を押すとタイマーがリセットされます。ウソじゃないです。",
         "Scramble Cube を押すとスクランブル手順が表示されます。多分ね。",
         ];
    Alert.show(messages.join("\n"), "SpeedCubeTimer について", Alert.OK);
}

private function scrambleCube():void {
    var pattern:Array = []; // [18, 2, 14, 6, 16, 1, 12, 10, 20, 2, 10, 22, 0, 14, 21, 0, 13, 9, 21, 12, 17, 8, 16, 9, 1, 12, 17, 2, 10, 21, 0];

    var tl:int = CUBE_SIZE;
    if ( (CUBE_SIZE & 1) != 0 ) tl--;

    var axsl:Array = new Array(tl);
    var axam:Array = new Array(0, 0, 0);

    var la:int;

    la = -1;
    pattern = new Array();

    while ( pattern.length < SEQ_LEN ) {
        var ax:int;
        do {
            ax = Math.floor(Math.random() * 3);
        } while ( ax == la );

        // Reset
        for ( var i:int = 0; i < tl; i++ ) axsl[i] = 0;
        axam[0] = axam[1] = axam[2] = 0;
        var moved:int = 0;

        do {
            var sl:int;
            do {
                sl = Math.floor(Math.random() * tl);
            } while ( axsl[sl] != 0 );
            var q:int = Math.floor(Math.random() * 3);

            if ( tl != CUBE_SIZE
                 || axam[q] * 2 < tl
                 || ( axam[0] * 2 == tl && axam[1] + axam[2] == 0 )
                 || ( axam[1] * 2 == tl && axam[2] + axam[0] == 0 )
                 || ( axam[2] * 2 == tl && axam[0] + axam[1] == 0 )
                 ) {
                axam[q]++;
                moved++;
                axsl[sl] = q + 1;
            }
        } while ( Math.floor(Math.random() * 3) == 0
                  && moved < tl
                  && moved + pattern.length < SEQ_LEN );

        for ( var sll:int = 0; sll < tl; sll++ ) {
            if ( axsl[sll] ) {
                var qq:int = axsl[sl] - 1;

                var sa:int = ax;
                var m:int = sll;
                if ( sll + sll + 1 >= tl ) {
                    sa += 3;
                    m = tl - 1 - m;
                    qq = 2 - qq;
                }
                pattern[pattern.length] = ( m * 6 + sa ) * 4 + qq;
            }
        }

        la = ax;
    }

    pattern[pattern.length] = Math.floor(Math.random() * 24);

    var s:String = "";
    var j:int;
    for ( var idx_i:int = 0; idx_i < pattern.length - 1; idx_i++ ) {
        if ( idx_i != 0 ) s += " ";
        var k:int = pattern[idx_i] >> 2;
        s += "DLBURFdlburf".charAt(k);
        j = pattern[idx_i] & 3;
        if ( j != 0 ) s += " 2'".charAt(j);
    }

    scramble.text = s;
}
