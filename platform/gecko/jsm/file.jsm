// vim:ft=javascript:
// // sample
// Components.classes["@mozilla.org/network/io-service;1"]
//           .getService(Components.interfaces.nsIIOService)
//           .getProtocolHandler("resource")
//           .QueryInterface(Components.interfaces.nsIResProtocolHandler)
//           .setSubstitution("coderepos.org", IO.newURI("file:///Users/cho45/coderepos/platform/gecko/jsm/"));
// Components.utils.import("resource://coderepos.org/file.jsm");

// var file = IO.getFile("Temp", "test.txt")
// File.write(file, "foobar");
// log(File.read(file));

EXPORTED_SYMBOLS = ["File"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const IO = Cc["@mozilla.org/io/scriptable-io;1"].getService();

File = {
	read : function (file, mode) {
		return File.open(file, mode || "text", function (s) {
			var res = [], str;
			while (str = s.readString(4096)) res.push(str);
			return res.join("");
		});
	},

	write : function (file, content, mode) {
		return File.open(file, mode || "write text", function (s) {
			s.writeString(content);
		});
	},

	open : function (file, mode, func) {
		var file = File.toFile(file);
		var write, charset, strm;
		mode.replace(/write|charset=(\S+)/g, function (_, v) {
			if (v) charset = v;
			else   write   = true;
			return "";
		});
		strm = IO[write ? "newOutputStream" : "newInputStream"](file, mode, charset);
		try {
			var ret = func.call(file, strm);
		} finally {
			strm.close();
		}
		return ret;
	},

	// toFile("path/to/file");
	// toFile(["Temp", "filename.txt"]);
	// toFile(fileobject);
	toFile : function (file) {
		if (typeof file == "string") {
			return IO.getFileWithPath(file);
		} else
		// 外部からのとだと(?) instanceof がつかえないようだ
		if (file.constructor.name == "Array") {
			return IO.getFile(file[0], file[1]);
		} else {
			return file;
		}
	}
};

// debug
function log () {
	var message = Array.prototype.slice.call(arguments).map(function (i) {
		if (typeof i == "object") return String(i);
		return uneval(i);
	});
	var console = Components.classes["@mozilla.org/consoleservice;1"]
	                        .getService(Components.interfaces.nsIConsoleService);
	try {
		console.logStringMessage(message);
	} catch(e) {
		Components.utils.reportError(message);
	}
};
