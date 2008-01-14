class psp {
    var running = false;

    var total, remain;          // time
    var cur, max;               // slide

    var timer_end;

    var mouse_down = false;

    function psp(mc) {
        load("bg");
        load("bar_time", { _x: 20, _y:125, _width: 0 });
        load("bar_task", { _x: 20, _y:145, _width: 0 });

        this.total = _root.total || 180;
        this.remain = this.total;

        this.max = _root.max || 100;
        this.cur = 1;

        mc.onEnterFrame = onEnterFrameHandler.bind(this);

        set_timer(this.remain);
        set_slide(1);

        Key.addListener({ onKeyUp: onKeyUp.bind(this) });
        Mouse.addListener({
            onMouseDown: (function() { this.mouse_down = true }).bind(this),
            onMouseUp: (function() { this.mouse_down = false }).bind(this)
        });
    }

    public function onKeyUp() {
        switch (Key.getCode()) {
        case 38:
            cmd("up");
            break;
        case 40:
            cmd("down");
            break;
        case 39:
            if (mouse_down) {
                this.running ? stop() : start();
            }
            else {
                cmd("right");
            }
            break;
        case 37:
            if (mouse_down) {
                reset();
            }
            else {
                cmd("left");
            }
            break;
        }
    }

    public function cmd(cmd) {
        if (mouse_down) cmd += "2";

        var lv = new LoadVars();
        lv.onData = cmd_callback.bind(this);
        lv.load("/cmd/"+cmd+"?"+(new Date()).getTime());
    }

    private function cmd_callback(page) {
        set_slide(int(page));
    }

    public function load(name, attr) {
        var id = nextId();
        var mc = _root.createEmptyMovieClip("mc_"+name, id);
        var mv = mc.attachMovie(name, name, 0);
        if (attr) {
            for (var i in attr) {
                mv[i] = attr[i];
            }
        }

        return _root[mc._name][mv._name];
    }

    public function create_mc(attr) {
        var id = nextId();
        var mc = _root.createEmptyMovieClip("mc_"+id, id);
        if (attr) {
            for (var i in attr) {
                mc[i] = attr[i];
            }
        }
        return mc;
    }

    public function set_time(percent) {
        _root.mc_bar_time.bar_time._width = 440 * percent;
    }

    public function set_task(percent) {
        _root.mc_bar_task.bar_task._width = 440 * percent;
    }

    public function start() {
        this.running = true;

        if (!this.timer_end) {
            this.timer_end = new Date();
            this.timer_end.setTime( (new Date()).getTime() + this.remain*1000 );
        }
    }

    public function stop() {
        this.running = false;
    }

    public function reset() {
        stop();
        this.remain = this.total;
        this.timer_end = null;

        set_time(0);
        set_timer(this.remain);
        set_slide(1);
    }

    public function set_timer(sec) {
        var hour = int(sec / (60 * 60)).toString();
        var min  = int((sec / 60) % 60 ).toString();
        sec  = (sec % 60).toString();

        if (hour.length < 2) hour = "0"+hour;
        if (min.length < 2) min = "0"+min;
        if (sec.length < 2) sec = "0"+sec;

        if (!arguments.callee.mc) {
            arguments.callee.mc = map(create_mc.bind(this), [
                { _x: 220+35*0, _y: 65 },
                { _x: 220+35*1, _y: 65 },
                { _x: 220+35*2, _y: 65 },
                { _x: 220+35*3 - 17, _y: 65 },
                { _x: 220+35*4 - 17, _y: 65 },
                { _x: 220+35*5 - 17, _y: 65 },
                { _x: 220+35*6 - 34, _y: 65 },
                { _x: 220+35*7 - 34, _y: 65 },
                { _x: 220+35*8 - 34, _y: 65 }
            ]);
        }

        var letters = [
            hour.charAt(0), hour.charAt(1), "colon",
            min.charAt(0), min.charAt(1), "colon",
            sec.charAt(0), sec.charAt(1)
        ];
        
        for (var i = 0, l = arguments.callee.mc.length; i < l; i++) {
            _root[arguments.callee.mc[i]._name].attachMovie("l_"+letters[i], "l_"+letters[i], 0);
        }
    }

    public function set_clock(date) {
        var hour = date.getHours().toString();
        var min  = date.getMinutes().toString();
        var sec  = date.getSeconds().toString();

        if (hour.length < 2) hour = "0"+hour;
        if (min.length < 2) min = "0"+min;
        if (sec.length < 2) sec = "0"+sec;

        if (!arguments.callee.mc) {
            arguments.callee.mc = map(create_mc.bind(this), [
                { _x: 120+12*0, _y: 80 },
                { _x: 120+12*1, _y: 80 },
                { _x: 120+12*2, _y: 80 },
                { _x: 120+12*3 - 6, _y: 80 },
                { _x: 120+12*4 - 6, _y: 80 },
                { _x: 120+12*5 - 6, _y: 80 },
                { _x: 120+12*6 - 12, _y: 80 },
                { _x: 120+12*7 - 12, _y: 80 },
                { _x: 120+12*8 - 12, _y: 80 }
            ]);
        }

        var letters = [
            hour.charAt(0), hour.charAt(1), "colon",
            min.charAt(0), min.charAt(1), "colon",
            sec.charAt(0), sec.charAt(1)
        ];
        
        for (var i = 0, l = arguments.callee.mc.length; i < l; i++) {
            _root[arguments.callee.mc[i]._name].attachMovie("s_"+letters[i], "s_"+letters[i], 0);
        }
    }

    public function set_slide(pos) {
        if (pos < 1 || this.max < pos) return;

        var percent = pos / this.max;
        set_task(percent);

        percent = (int(percent * 100)).toString();
        while (percent.length < 3) percent = " "+percent;

        var line = pos.toString() + "/" + this.max + percent + "%";
        var lines = line.split("").reverse();

        for (var i = 0, l = arguments.callee.mc.length; i < l; i++) {
            _root[arguments.callee.mc[i]._name].removeMovieClip();
        }

        arguments.callee.mc = map(create_mc.bind(this), [
            { _x: 443-24*0, _y: 177 },
            { _x: 443-24*1, _y: 177 },
            { _x: 443-24*2, _y: 177 },
            { _x: 443-24*3, _y: 177 },
            { _x: 443-24*4, _y: 177 },
            { _x: 443-24*5, _y: 177 },
            { _x: 443-24*6, _y: 177 },
            { _x: 443-24*7, _y: 177 },
            { _x: 443-24*8, _y: 177 },
            { _x: 443-24*9, _y: 177 },
            { _x: 443-24*10, _y: 177 },
            { _x: 443-24*11, _y: 177 },
            { _x: 443-24*12, _y: 177 },
            { _x: 443-24*13, _y: 177 },
            { _x: 443-24*14, _y: 177 },
            { _x: 443-24*15, _y: 177 },
            { _x: 443-24*16, _y: 177 },
            { _x: 443-24*17, _y: 177 },
            { _x: 443-24*18, _y: 177 }
        ]);
        
        for (var i = 0, l = arguments.callee.mc.length; i < l; i++) {
            var letter = lines[i];

            if (typeof(letter) == "undefined") continue;
            if (letter == " ") continue;
            if (letter == "/") letter = "slash";
            if (letter == "%") letter = "percent";

            _root[arguments.callee.mc[i]._name].attachMovie("m_"+letter, "m_"+letter, 0);
        }
    }

    public function onEnterFrameHandler() {
        if (this.running) {
            var remain = this.timer_end.getTime() - (new Date()).getTime();
            if (remain < 0) {
                this.running = false;
                remain = 0;
            }

            set_timer(this.remain = int(remain/1000));
            set_time( (this.total - this.remain) / this.total );
        }

        set_clock(new Date());
    }

    public function nextId() {
        var counter = function () {
            var i = 0;
            return function () { return ++i };
        }
        return (arguments.callee.nextId || (arguments.callee.nextId = counter()))();
    }

    public function map(fn, list) {
        var r = [];
        for (var i = 0, l = list.length; i < l; i++) {
            r.push(fn(list[i]));
        }
        return r;
    }

    public function grep(fn, list) {
        var r = [];
        for (var i = 0, l = list.length; i < l; i++) {
            if (fn(list[i])) r.push(list[i]);
        }
        return r;
    }

    static function main(mc) {
        Function.prototype.bind = function(object) {
            var __method = this;
            var __args = arguments.slice(1);
            return function() { return __method.apply(object, __args.concat(arguments)) }
        }

        var app = new psp(mc);
    }
}
