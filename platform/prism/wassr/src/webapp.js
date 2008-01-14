// kick the api.
function startup() {
    Wassr.startup();
}
function shutdown() {
    Wassr.shutdown();
}
function dropFiles(uris) {
}

var Wassr = {
    _timer : null,
    _url : 'http://wassr.jp/my/',
    _last_message_ctime: '0000-00-00 00:00:00',

    _unescapeHTML : function (src) {
        var div = host.getBrowser().contentWindow.document.createElement("DIV");
        div.innerHTML = src;
        return div.textContent;
    },

    _periodicalUpdate : function() {
        try {
            // Ajax.
            var xhr = new XMLHttpRequest;
            xhr.onreadystatechange = function () {
                if (xhr.readyState != 4) { return; }
                try {
                    if (xhr.status != 200) { throw "Problem retrieving XML data"; }

                    var document = host.getBrowser().contentWindow.document;

                    // update html.
                    var d = document.getElementById('EntryMessages');
                    if (d) {
                        d.innerHTML = xhr.responseText;
                    }

                    // update unread num.
                    var xhr_unread = xhr.responseXML.getElementById('unread_reply_num');
                    var doc_unread = document.getElementById('unread_reply_num');
                    if (xhr_unread && doc_unread) {
                        doc_unread.innerHTML = xhr_unread.innerHTML;
                    }

                    // growl
                    try {
                        var i, l;
                        // ctime
                        var ctimes = document.getElementsByClassName("MsgDateTime");
                        for (i=0,l=ctimes.length; i<l; i++) {
                            ctimes[i] = ctimes[i].innerHTML.replace(/\(.+\)/, '');
                        }

                        // messages
                        var messages = document.getElementsByClassName("message");
                        for (i=0,l=messages.length; i<l; i++) {
                            messages[i] = messages[i].innerHTML.replace(/\s+|<[^>]+>/g, '')
                        }

                        // usernames
                        var usernames = document.getElementsByClassName('messagefoot');
                        for (i=0,l=usernames.length; i<l; i++) {
                            usernames[i] = usernames[i].childNodes[1].innerHTML;
                        }

                        // do notify
                        for (i=0,l=ctimes.length; i<l; i++) {
                            if (ctimes[i] > Wassr._last_message_ctime) {
                                host.showAlert(
                                    host.getResource("mail28.png"),
                                    "Wassr",
                                    Wassr._unescapeHTML(usernames[i]) + '  :  ' + Wassr._unescapeHTML(messages[i])
                                );
                            }
                        }

                        // save last time.
                        Wassr._last_message_ctime = ctimes[0];
                    } catch (e) {
                        host.log("NOTIFY ERROR");
                        host.log(e);
                    }
                } catch (e) {
                    host.log("xhr ERROR");
                    host.log(e);
                }
            };
            xhr.overrideMimeType('text/xml');
            xhr.open("GET", Wassr._url, true);
            xhr.setRequestHeader('X-Requested-With', 'XMLHttpRequest');
            xhr.send(null);
        } catch(e) {
            host.log(e);
        };
    },

    // disable ajax on click link.
    _operate_dom : function () {
        try {
            // operating DOM.
            host.log("OPERATING NOW");

            var document = host.getBrowser().contentWindow.document;
            if ( ! document.getElementById('PrismMenu') ) {
                host.log("Insert!");
                var div = document.createElement('div');
                div.id = 'PrismMenu';
                div.innerHTML = '<a href="/my/">MyPage</a>';
                document.getElementById('Side').parentNode.appendChild(div);
            }
        } catch(e) {
            host.log(e);
        };

        var tagged_links = host.getBrowser().contentWindow.document.getElementsByClassName('taggedlink')
        for (var i=0; i<tagged_links.length; i++) {
            tagged_links[i].onclick = null;
        }
    },

    startup : function() {
        setInterval(Wassr._operate_dom, 500);
        this._timer = setInterval(Wassr._periodicalUpdate, 60*1000);
    },

    shutdown : function() {
        if (this._timer) {
            clearInterval(this._timer);
        }
    }
};

