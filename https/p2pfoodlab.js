/*  

    P2P Food Lab 

    P2P Food Lab is an open, collaborative platform to develop an
    alternative food value system.

    Copyright (C) 2013  Sony Computer Science Laboratory Paris
    Author: Peter Hanappe

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

function show_hide(id)
{
        if (document.getElementById) {
                var short_text = document.getElementById(id + "_short");
                var long_text = document.getElementById(id);
                if (long_text.className.indexOf("hidden") >= 0) {
                        long_text.className = long_text.className.replace("hidden", 
                                                                          "visible"); 
                        if (short_text)
                                short_text.className = short_text.className.replace("visible",
                                                                                    "hidden"); 
                } else if (long_text.className.indexOf("visible") >= 0) {
                        long_text.className = long_text.className.replace("visible", 
                                                                          "hidden"); 
                        if (short_text)
                                short_text.className = short_text.className.replace("hidden",
                                                                                    "visible"); 
                }
        }
        return false;
}

function show_hide_thread(id, show, hide)
{
        if (document.getElementById) {
                var div = document.getElementById(id);
                if (!div) return;
                var a = document.getElementById("showhide_" + id);
                if (div.className.indexOf("hidden") >= 0) {
                        div.className = div.className.replace("hidden", 
                                                              "visible"); 
                        if (a) a.innerHTML = hide;
                } else if (div.className.indexOf("visible") >= 0) {
                        div.className = div.className.replace("visible", 
                                                              "hidden"); 
                        if (a) a.innerHTML = show;
                }
        }
        return false;
}

function P2PFoodLab(root)
{
        if (root)
                this.root = root;
        else
                this.root = p2pfoodlab.root

                        this.newXMLHttpRequest = function(userData, callback) {
                        var xmlhttp = undefined;

                        try {
                                xmlhttp = new ActiveXObject("Msxml2.XMLHTTP");
                        } catch (e) {
                                try {
                                        xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
                                } catch (e2) {
                                        xmlhttp = undefined;
                                }
                        }
                        if ((xmlhttp == undefined) 
                            && (typeof XMLHttpRequest != 'undefined')) {
                                xmlhttp = new XMLHttpRequest();
                        }
                        xmlhttp.userData = userData;
                        xmlhttp.callback = callback;
                        return xmlhttp;
                }

        this.handleStateChange = function(xmlhttp) {
                if (xmlhttp.readyState == 4) {
                        if (xmlhttp.status == 200) {
                                this.handleResponse(xmlhttp);
                        } else {
                                //alert("Request for " + xmlhttp._url + " failed: Status " + xmlhttp.status);
                        }
                        delete xmlhttp;
                }
        }

        this.handleResponse = function(xmlhttp) {
                //alert(xmlhttp.response);
                response = JSON.parse(xmlhttp.response);
                if (xmlhttp.callback) 
                        xmlhttp.callback(xmlhttp.userData, response);
                delete xmlhttp;
        }
    
        this.get = function(url, userData, callback) {
                var self = this;
                var xmlhttp = this.newXMLHttpRequest(userData, callback);
                xmlhttp._url = this.root + "/" + url;
                //alert(xmlhttp._url);
                xmlhttp.async = (callback == undefined)? false : true;
                xmlhttp.open("GET", this.root + "/" + url, xmlhttp.async);
                //alert(this.root + "/" + url);
                if (xmlhttp.async) {
                        xmlhttp.onreadystatechange = function() {
                                self.handleStateChange(xmlhttp);
                        }
                }
                xmlhttp.send();
                if (!xmlhttp.async) {
                        var r = JSON.parse(xmlhttp.response);
                        delete xmlhttp;
                        return r;
                }
        }

        this.rpc = function(request, userData, callback) {
                var self = this;
                var xmlhttp = this.newXMLHttpRequest(userData, callback);
                xmlhttp.async = (callback == undefined)? false : true;
                xmlhttp.open("PUT", p2pfoodlab.root + "/rpc.php", xmlhttp.async);
                xmlhttp.onreadystatechange = function() {
                        self.handleStateChange(xmlhttp);
                }
                xmlhttp.send(JSON.stringify(request));
                if (!xmlhttp.async) {
                        //alert(xmlhttp.response);
                        var r = JSON.parse(xmlhttp.response);
                        delete xmlhttp;
                        return r;
                }
        }

        this.put = function(url, obj, userData, callback) {
                var self = this;
                var xmlhttp = this.newXMLHttpRequest(userData, callback);
                xmlhttp.async = (callback == undefined)? false : true;
                xmlhttp.open("PUT", this.root + "/" + url, xmlhttp.async);
                xmlhttp.onreadystatechange = function() {
                        self.handleStateChange(xmlhttp);
                }
                xmlhttp.send(JSON.stringify(obj));
                if (!xmlhttp.async) {
                        var r = JSON.parse(xmlhttp.response);
                        delete xmlhttp;
                        return r;
                }
        }

        this.selectSensorbox = function(id, value)
        {
                this.rpc({ "op": "select_sensorbox",
                                        "gid": id,
                                        "select": value },
                        null, null);
        }

        this.selectDatastream = function(gid, did, value)
        {
                this.rpc({ "op": "select_datastream",
                                        "gid": gid,
                                        "did": did,
                                        "select": value },
                        null, null);
        }

        this.removeDoc = function(postid, docid)
        {
                this.rpc({ "op": "remove_doc", "post_id": postid, "doc_id": docid },
                         null, null);
        }
}


function updateInterfaceLanguageCallback(userDate, response)
{
        window.location.reload(true);
}

function updateInterfaceLanguage()
{
        request = { "op": "set_locale", 
                    "locale": document.forms["set_locale"].lang.value };
        new P2PFoodLab().rpc(request, null, updateInterfaceLanguageCallback);
}

function updateLanguageCallback(userDate, response)
{
        window.location.reload(true);
}

function updateLanguage(code, name)
{
        value = document.getElementsByName(name)[0].checked;
        new P2PFoodLab().rpc({ "op": "set_lang",
                                "lang": code,
                                "select": value },
                null, updateLanguageCallback);
}


// Thanks to Scott Andrew LePera
// (http://www.scottandrew.com/weblog/articles/cbs-events) and
// Christian Heilmann
// (http://onlinetools.org/articles/unobtrusivejavascript/chapter4.html)
// for the setEventHandler() and removeEventHandler() functions.
function setEventHandler(obj, evType, fn)
{ 
        if (obj.addEventListener){ 
                obj.addEventListener(evType, fn, false); 
                return true; 

        } else if (obj.attachEvent){ 
                var r = obj.attachEvent("on"+evType, fn); 
                return r; 

        } else { 
                return false; 
        } 
}

function addText(elem, text) 
{
    var textNode = document.createTextNode(text);
    elem.appendChild(textNode);
    return textNode;
}

function addLink(elem, anchorText, href, className) 
{
    var a = document.createElement("A");
    a.className = className;
    a.setAttribute("href", href);
    a.appendChild(document.createTextNode(anchorText));
    elem.appendChild(a);
    return a;
}

function addEventLink(elem, anchorText, handler, className) 
{
    var a = document.createElement("A");
    a.className = className;
    a.setAttribute("href", "javascript:void(0)");
    a.appendChild(document.createTextNode(anchorText));
    a.onclick = function() { return false; }
    a.onmousedown = function() { return false; }
    setEventHandler(a, "click", handler);
    elem.appendChild(a);
    return a;
}

function addEventImage(elem, src, alt, handler, className) 
{
        var img = document.createElement("IMG");
        img.src = src;
        img.alt = alt;
        img.title = alt;
        img.className = className;

        var a = document.createElement("A");
        a.className = className;
        a.setAttribute("href", "javascript:void(0)");
        a.appendChild(img);
        setEventHandler(a, "click", handler);
        elem.appendChild(a);

        return a;
}

function ComponentProto()
{
        this.setStyle = function(s) {
                this.style = s;
                this.updateStyle();
        }

        this.setVisible = function(value) {
                this.visible = value;
                this.updateStyle();
        }

        this.isVisible = function() {
                return this.visible;
        }

        this.toggleVisibility = function() {
                if (this.visible) {
                        this.visible = false;
                } else {
                        this.visible = true;
                }
                this.updateStyle();
        }

        this.updateStyle = function(value) {
                if (this.visible) {
                        this.div.className = this.style + " visible";
                } else {
                        this.div.className = this.style + " hidden";
                }
        }

        this.addText = function(text, style) {
                var textNode = document.createTextNode(text);
                if (style == undefined) {
                        this.div.appendChild(textNode);
                } else {
                        var span = document.createElement("SPAN");
                        span.className = style;
                        span.appendChild(textNode);
                        this.div.appendChild(span);
                }
                return textNode;
        }

        this.addBreak = function() {
                this.div.appendChild(document.createElement("BR"));
        }

        this.addLink = function(anchorText, href, className) {
                var a = document.createElement("A");
                a.className = className;
                a.setAttribute("href", href);
                a.appendChild(document.createTextNode(anchorText));
                this.div.appendChild(a);
                return a;
        }

        this.addEventLink = function(anchorText, handler, className) {
                var a = document.createElement("A");
                a.className = className;
                a.setAttribute("href", "javascript:void(0)");
                a.appendChild(document.createTextNode(anchorText));
                a.onclick = function() { return false; }
                a.onmousedown = function() { return false; }
                setEventHandler(a, "click", handler);
                this.div.appendChild(a);
                return a;
        }

        this.addImage = function(src, alt, className) {
                var img = document.createElement("IMG");
                img.src = src;
                if (alt) {
                        img.alt = alt;
                        img.title = alt;
                } 
                if (className) {
                        img.className = className;
                }
                this.div.appendChild(img);
                return img;
        }

        this.addComponent = function(c) {
                //alert("Component.addComponent: " + this + ": " + c);
                this.components.push(c);
                this.div.appendChild(c.div);
                c.parent = this;
        }

        this.appendChild = function(element) {
                this.div.appendChild(element);
        }

        this.removeComponents = function() {
                this.components = new Array();
                while (this.div.hasChildNodes()) {
                        this.div.removeChild(this.div.firstChild);
                }
        }

        this.removeComponent = function(c) {
                var i = 0;
                while (i < this.components.length) {
                        component = this.components[i];
                        if (component == c) {
                                this.components.splice(i, 1);
                                break;
                        }
                        i++;
                }
                i = 0;
                while (i < this.div.childNodes.length) {	    
                        e = this.div.childNodes[i++];
                        if (e == c.div) {
                                this.div.removeChild(e);
                                break;
                        }
                }	
                c.parent = undefined;
        }

        this.countComponents = function() {
                return this.components.length;
        }

        this.getComponent = function(id) {
                var i = 0;
                while (i < this.components.length) {
                        component = this.components[i];
                        if (id == component.id) {
                                return component;
                        }
                        i++;
                }
        }

        this.getComponentAt = function(index) { 
                return this.components[index];
        }

        this.addComponentAt = function(id, c) {
                var i = 0;
                while (i < this.components.length) {
                        component = this.components[i];
                        if (id == component.id) {
                                component.addComponent(c);
                                break;
                        }
                        i++;
                }
        }

        this.setEventHandler = function(evType, fn) {
                setEventHandler(this.div, evType, fn);
        }

}


function Component(id, style)
{
        this.id = id;
        this.div = document.createElement("DIV");
        this.div.id = id;
        this.div.className = style;
        this.style = style;
        this.visible = true;
        this.parent = undefined;
        this.components = new Array();
}
Component.prototype = new ComponentProto();


function TextArea(id, style, editable, createform)
{
        var self = this;

        this.base = Component;
        this.base("TextArea_" + id, style);
        this.text = ""; 
        this.editable = editable;
        this.textarea = undefined;
        this.createform = createform;
        this.form = undefined;
        this.listeners = new Array();

        this.addListener = function(listener) { 
                this.listeners.push(listener);
        }

        this.callListeners = function() { 
                for (var i = 0; i < this.listeners.length; i++) {
                        if (this.listeners[i].handleChange)
                                this.listeners[i].handleChange(this);
                }
        }

        this.getText = function() {
                return this.text;
        }

        this.setText = function(text) {
                this.text = text;
                if (this.editable && this.textarea)
                        this.textarea.value = text;
                else this.displayFixedText();
        }

        this.editText = function() {
                if (self.editable && !this.textarea) {
                        self.displayEditableText();
                }
        }

        this.textChanged = function() {
                self.text = self.textarea.value;
                self.callListeners();
        }

        this.select = function() {
                if (this.textarea) 
                        this.textarea.select();
        }

        this.displayFixedText = function() {
                this.removeComponents();
                this.textarea = undefined;
                this.div.innerHTML = this.text; 
        }

        this.displayEditableText = function() {
                this.removeComponents();

                this.textarea = document.createElement("TEXTAREA");
                this.textarea.className = this.style;
                this.textarea.setAttribute("name", "text");
                if (this.text != undefined) {
                        this.textarea.value = this.text; 
                } else {
                        this.textarea.value = ""; 
                }
                this.textarea.select();
                setEventHandler(this.textarea, "keyup", this.textChanged);
        
                if (this.createform) {
                        this.form = document.createElement("FORM");
                        this.form.onsubmit = function() { return false; }
                        setEventHandler(this.form, "submit", this.saveText);
                        this.form.appendChild(this.textarea);
                        this.div.appendChild(this.form);
                } else {
                        this.form = undefined;
                        this.div.appendChild(this.textarea);
                }
        }

        if (this.editable)
                this.displayEditableText();
        else this.displayFixedText();
}
TextArea.prototype = new ComponentProto();


function RemoveDoc(editor, docid, postid)
{
        var self = this;
        this.editor = editor;
        this.docid = docid;
        this.postid = postid;

        this.removeLocalDoc = function() {
                self.editor.removeLocalDoc(self.docid);
        }

        this.removeServerDoc = function() {
                self.editor.removeServerDoc(self.postid, self.docid);
        }
}

function NoteEditor(id, parent, thread, replyto, lang, lang_opt)
{
        var self = this;

        this.base = Component;
        this.base(id, "editor visible");

        this.handleChange = function() {
                self.updateTextLength();
        }

        this.updateTextLength = function() {
                var len = this.textarea.getText().length;
                this.textlen.removeComponents();
                if (len > 2000) {
                        this.textlen.addText("" + len + " > 2000 !");
                        this.textlen.setStyle("editor_textlen warning");
                } else {
                        this.textlen.addText("" + len + "/2000");
                        this.textlen.setStyle("editor_textlen");
                }
        }

        if (replyto) 
                this.addText("Reply:", "editor_title");
        this.form = document.createElement("FORM");
        this.form.method = "POST";
        this.form.action = p2pfoodlab.root + "/publish.php";
        this.form.enctype = "multipart/form-data";

        this.docs = 0;
    
        var input = document.createElement("INPUT");
        input.setAttribute("type", "hidden");
        input.setAttribute("name", "op");
        input.setAttribute("value", "postnote");
        this.form.appendChild(input);

        input = document.createElement("INPUT");
        input.setAttribute("type", "hidden");
        input.setAttribute("name", "url");
        input.setAttribute("value", window.location.href);
        this.form.appendChild(input);

        if (thread) {
                input = document.createElement("INPUT");
                input.setAttribute("type", "hidden");
                input.setAttribute("name", "thread");
                input.setAttribute("value", thread);
                this.form.appendChild(input);
        }
        if (replyto) {
                input = document.createElement("INPUT");
                input.setAttribute("type", "hidden");
                input.setAttribute("name", "replyto");
                input.setAttribute("value", replyto);
                this.form.appendChild(input);
        }

        this.textarea = new TextArea(id, "editor_text", true);
        this.textarea.addListener(this);
        this.form.appendChild(this.textarea.div);

        this.textlen = new Component("textlen", "editor_textlen");
        this.textlen.addText("0/2000");
        this.form.appendChild(this.textlen.div);

        this.docsdiv = document.createElement("DIV");
        this.docsdiv.className = "editor_docs";
        this.form.appendChild(this.docsdiv);




        if (!lang) {
                this.lang = document.createElement("INPUT");
                this.lang.setAttribute("type", "hidden");
                this.lang.setAttribute("name", "lang");
                this.lang.setAttribute("value", "en");
                this.form.appendChild(this.lang);
        } else if (lang_opt && (lang_opt.length > 1)) {
                for (i = 0; i < lang_opt.length; i++) {
                        input = document.createElement("INPUT");
                        input.setAttribute("type", "radio");
                        input.setAttribute("name", "lang");
                        input.setAttribute("value", lang_opt[i]);
                        if (lang == lang_opt[i]) 
                                input.setAttribute("checked", "checked");
                        this.form.appendChild(input);
                        addText(this.form, lang_opt[i]);
                }
        } else {
                this.lang = document.createElement("INPUT");
                this.lang.setAttribute("type", "hidden");
                this.lang.setAttribute("name", "lang");
                this.lang.setAttribute("value", lang);
                this.form.appendChild(this.lang);
        }

        this.div.appendChild(this.form);

        var div = document.createElement("DIV");
        div.className = "editor_docs_errmsg";
        div.id = "editor_docs_errmsg";
        this.div.appendChild(div);

        this._sendReply = function() {
                self.form.submit();
        }

        this.docID = 0;

        this.removeDocDiv = function(id) {
                        //alert("id = " + id);
                for (i = 0; i < this.docsdiv.childNodes.length; i++) {	    
                        e = this.docsdiv.childNodes[i];
                        //alert("div id = " + e.id);
                        if (e.id && (e.id == id)) {
                                this.docsdiv.removeChild(e);
                                this.docs--;
                                break;
                        }
                }
        }

        this.removeLocalDoc = function(id) {
                this.removeDocDiv("doc_loc_" + id);
        }

        this.removeServerDoc = function(postid, docid) {
                this.removeDocDiv("doc_serv_" + docid);
                new P2PFoodLab().removeDoc(postid, docid);
        }

        this._showIcon = function(e) {
                self.showIcon(e);
        }

        this.showIcon = function(e) {
                var fileInput = e.target;
                var inputId = e.target.id;
                var file = e.target.files[0];

                if (file.type == "image/jpeg") {

                        var reader = new FileReader();
                        reader.onload = function(e) {
                                img = document.getElementById(inputId + "_img");
                                img.src = reader.result;
                                img.className = "note_icon visible";
                                fileInput.className = "hidden";
                        }
                        reader.readAsDataURL(file);	
                } else {
                        this.removeDocDiv(inputId);
                        var d = document.getElementById("editor_docs_errmsg");
                        d.innerHTML = "Only JPEG images are currently supported.";
                }
        }

        this.loadPost = function(post) {
                this.post = post;
                this.textarea.setText(post.text);
                input = document.createElement("INPUT");
                input.setAttribute("type", "hidden");
                input.setAttribute("name", "post");
                input.setAttribute("value", post.id);
                this.form.appendChild(input);
                this.updateTextLength();
                
                for (i = 0; i < this.post.docs.length; i++) {
                        var doc = this.post.docs[i];
                        this.addServerDoc(post, doc);
                }
        }

        this.addLocalDoc = function() {
                if (this.docs == 4)
                        return;

                var d = document.getElementById("editor_docs_errmsg");
                d.innerHTML = "";

                var div = document.createElement("DIV");
                div.className = "editor_doc";
                div.id = "doc_loc_" + this.docID;

                var icondiv = document.createElement("DIV");
                icondiv.className = "editor_icon";
                icondiv.id = "doc_" + this.docID + "_icon";
                div.appendChild(icondiv);

                var img = document.createElement("IMG");
                img.className = "note_icon hidden";
                img.id = "doc_loc_" + this.docID + "_img";
                img.src = "";
                icondiv.appendChild(img);

                var rmdiv = document.createElement("DIV");
                rmdiv.className = "editor_rmdoc";
                rmdiv.id = "doc_" + this.docID + "_rm";
                div.appendChild(rmdiv);

                var remover = new RemoveDoc(this, this.docID, null);
                addEventImage(rmdiv, p2pfoodlab.root + "/close.png", 
                              "remove", remover.removeLocalDoc, ""); 

                var input = document.createElement("INPUT");
                input.setAttribute("type", "file");
                input.setAttribute("id", "doc_loc_" + this.docID);
                input.setAttribute("name", "docs[]");
                input.className = "editor_filechooser";
                setEventHandler(input, "change", this._showIcon);
                div.appendChild(input);

                this.docsdiv.appendChild(div);
                this.docID++;
                this.docs++;

                //alert("input.click");
                input.click();
        }

        this.addServerDoc = function(post, doc) {
                var div = document.createElement("DIV");
                div.className = "editor_doc";
                div.id = "doc_serv_" + doc.id;

                var icondiv = document.createElement("DIV");
                icondiv.className = "editor_icon";
                icondiv.id = "doc_" + doc.id + "_icon";
                div.appendChild(icondiv);

                var img = document.createElement("IMG");
                img.className = "note_icon";
                img.id = "doc_" + doc.id + "_img";
                img.src = p2pfoodlab.root + "/docs/" + doc.path + "-thumb.jpg";
                icondiv.appendChild(img);

                var rmdiv = document.createElement("DIV");
                rmdiv.className = "editor_rm";
                rmdiv.id = "doc_" + doc.id + "_rm";
                div.appendChild(rmdiv);

                var remover = new RemoveDoc(this, doc.id, post.id);
                addEventImage(rmdiv, p2pfoodlab.root + "/close.png", 
                              "remove", remover.removeServerDoc, ""); 

                this.docsdiv.appendChild(div);
                this.docID++;
                this.docs++;
        }

        this._addLocalDoc = function() {
                self.addLocalDoc();
        }

        this._cancel = function() {
                self.cancel();
        }

        this.cancel = function() {
                this.note.removeChild(this.div);
                if (this.post) {
                        // FIXME: cheap...
                        window.location.reload(true);
                }
        }

        this.addEventLink("Add a photo", this._addLocalDoc, "editor_button"); 
        this.addEventLink("Send", this._sendReply, "editor_button"); 
        var s = document.createElement("SPAN");
        s.className = "separator";
        this.appendChild(s);
        this.addEventLink("Cancel", this._cancel, "editor_button"); 

        this.note = document.getElementById(parent);
        this.note.appendChild(this.div);

        this.textarea.select();    
}
NoteEditor.prototype = new ComponentProto();


function do_reply(thread, post, lang)
{
        var editor = document.getElementById("editor_" + post);
        if (!editor) {
                new NoteEditor("editor_reply_" + post, "post_" + post, 
                               thread, post, lang, undefined);
        }
}

function do_newnote(lang, lang_opt)
{
        var editor = document.getElementById("editor_newnote");
        if (!editor) {
                new NoteEditor("editor_newnote", "newnote", 
                               undefined, undefined, lang, lang_opt);
        }
}

function do_edit(id)
{
        var div = document.getElementById("post_" + id);
        if (!div) return;
        while (div.firstChild) {
                div.removeChild(div.firstChild);
        } 
        var post = new P2PFoodLab().get("rest/post/" + id + ".json");
        var e = new NoteEditor("editor_" + id, "post_" + id, 
                               undefined, undefined, undefined, [ ]);
        e.loadPost(post);
}

function show_photos(rootId, photos, selected, handler)
{
        var icons = document.getElementById(rootId + "_icons");
        if (icons) {
                icons.className = "note_icons hidden";
        }
        var root = document.getElementById(rootId);
        if (root) {
                var slideshow = new Slideshow(rootId, 
                                              photos, null, 790, 620, 
                                              handler);
                slideshow.selectSlide(selected);
        }
}

function hide_photos(rootId)
{
        var icons = document.getElementById(rootId + "_icons");
        if (icons) {
                icons.className = "note_icons visible";
        }
        var root = document.getElementById(rootId);
        if (root) {
                var box = document.getElementById(rootId + "_Slideshow");
		root.removeChild(box);
                root.style.height = "100%"; // FIXME
        }
}

function getgroup(id) 
{
        for (i = 0; i < _groups.length; i++) {
                if (_groups[i].id == id)
                        return _groups[i];
        }
}

function changegroup() 
{
        var gid = document.forms['groupselector'].group.value;
        var show = document.forms['groupselector'].show.checked;
        _visible[gid] = show; 
        new P2PFoodLab().selectSensorbox(gid, show);
}

function DatastreamSelector(gid, did, input) 
{
        this.handler = function() {
                if (!_datastream[gid]) 
                        _datastream[gid] = [];
                _datastream[gid][did] = input.checked;
                new P2PFoodLab().selectDatastream(gid, did, input.checked);
        };
}

function showgroup() 
{
        var gid = document.forms['groupselector'].group.value;
        var show = _visible[gid]; 
        if (show) document.forms['groupselector'].show.checked = true;
        else document.forms['groupselector'].show.checked = false;

        var form = document.forms['dataselector'];
        while (form.firstChild) {
                form.removeChild(form.firstChild);
        }        
        var g = getgroup(gid);
        if (!g) return;
        
        for (i = 0; i < g.datastreams.length; i++) {
                var did = g.datastreams[i].id;
                var div = document.createElement("DIV");
                div.className = "dataselector";
                var label = g.datastreams[i].name;
                var text = document.createTextNode(label);
                var input = document.createElement("INPUT");
                input.type = "checkbox";
                input.className = "dataselector";
                input.value = g.datastreams[i].id;
                if (_datastream[gid] && 
                    _datastream[gid][did])
                        input.checked = _datastream[gid][did];

                var selector = new DatastreamSelector(gid, did, input);
                setEventHandler(input, "change", selector.handler);

                div.appendChild(text);
                div.appendChild(input);
                form.appendChild(div);

                
        }
}
