

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

function SlideSelector(slideshow, i)
{
        var self = this;
        this.slideshow = slideshow;        
        this.i = i;        

        this.select = function() {
                self.slideshow.selectSlide(self.i);
        }
}

function Slideshow(rootId, slides, captions, width, height, handler)
{
        var self = this;
        this.rootId = rootId;
        this.slides = slides;
        this.captions = captions;
        this.self = this;
        this.curSlide = 0;
        this.preLoad = [];
        this.slide = null;
        this.slideSelectors = [];
        this.width = width;
        this.height = height;
        this.handler = handler;

        this.preloadSlides = function() {
                this.preLoad = [];
                for (i = 0; i < this.slides.length; i++) {
                        this.preLoad[i] = new Image();
                        if ((typeof this.slides[i]) == "string")
                                src = this.slides[i];
                        else 
                                src = this.slides[i].small;
                        this.preLoad[i].src = src;
                }
                setEventHandler(this.preLoad[0], "load", this._doTransition);
        }

        this.nextSlide = function() {
                if (self) self.selectSlide(self.curSlide + 1);
                else this.selectSlide(this.curSlide + 1);
        }

        this.prevSlide = function() {
                if (self) self.selectSlide(self.curSlide - 1);
                else this.selectSlide(this.curSlide - 1);
        }

        this.selectSlide = function(i) {
                this.curSlide = i;
                if (this.curSlide < 0) 
                        this.curSlide = 0;
                if (this.curSlide >= this.slides.length) 
                        this.curSlide = this.slides.length - 1;
                this.doTransition();
        }

        this._doTransition = function() {
                self.doTransition();
        }

        this.doTransition = function() {
                var img = this.preLoad[this.curSlide];
                var w = img.width;
                var h = img.height;
                var tw = this.imageWidth;
                var th = this.imageHeight;
                
                if (w > tw) {
                        h = h * tw / w;
                        w = tw;
                }
                if (h > th) {
                        w = w * th / h;
                        h = th;
                }
                this.slide.width = w;
                this.slide.height = h;
                this.slide.style.top = "" + ((th - h) / 2) + "px";
                this.slide.style.left = "" + ((tw - w) / 2) + "px";
                this.slide.src = this.preLoad[this.curSlide].src;
                this.displayCaption();
                this.updateSlideNumbers();
        }

        this.insertLink = function(p, className, anchor, id, handler) {
                var a = document.createElement("A");
                a.className = className;
                a.id = id;
                a.setAttribute("href", "javascript:void(0)");
                a.appendChild(document.createTextNode(anchor));
                p.appendChild(a);        
                a.onclick = function() { return false; }
                a.onmousedown = function() { return false; }
                setEventHandler(a, "click", handler);
                return a;
        }

        this.insertSlideNumbers = function() {
                if (this.slides.length <= 1)
                        return;

                this.insertLink(this.ctrl, "Arrow", "<", 
                                this.rootId + "_prevSlide", 
                                this.prevSlide);
                
                for (i = 0; i < this.slides.length; i++) {
                        var sel = new SlideSelector(this, i);
                        var link = this.insertLink(this.ctrl, "Number", "" + (i + 1), 
                                                   this.rootId + "_selectSlide_" + i, 
                                                   sel.select);
                        sel.link = link;
                        this.slideSelectors[i] = sel;
                        if (i == this.curSlide) 
                                link.className = "SelectedNumber";
                }
                
                this.insertLink(this.ctrl, "Arrow", ">", 
                                this.rootId + "_nextSlide", this.nextSlide);
        }

        this.updateSlideNumbers = function() {
                if (this.slides.length <= 1)
                        return;
                for (i = 0; i < this.slides.length; i++)
                        this.slideSelectors[i].link.className = "Number";
                this.slideSelectors[this.curSlide].link.className = "SelectedNumber";
        }

        this.displayCaption = function() {
                if (this.captions) {
                        var div = document.getElementById(this.rootId + "_CaptionBox");
                        div.innerHTML = this.captions[this.curSlide];
                }
        }
        
        this._onSlideClicked = function(e) {
                self.handler(self);
        }

        this.createShow = function(rootId) {
                var root = document.getElementById(rootId);
                if (!root) return -1;
                root.style.width = this.width + "px";
                root.style.height = this.height + "px";

                var slideshow = document.createElement("DIV");
                slideshow.className = "Slideshow";
                slideshow.id = this.rootId + "_Slideshow";
                root.appendChild(slideshow);
                
                var pic = document.createElement("DIV");
                pic.className = "PictureBox";
                pic.id = this.rootId + "_PictureBox";
                slideshow.appendChild(pic);

                var img = document.createElement("IMG");
                img.className = "SlideshowImage";
                this.slide = img;

                if (this.handler) {
                        var a = document.createElement("A");
                        a.setAttribute("href", "javascript:void(0)");
                        a.onclick = function() { return false; }
                        a.onmousedown = function() { return false; }
                        setEventHandler(a, "click", this._onSlideClicked);

                        a.appendChild(img);
                        pic.appendChild(a);

                } else {
                        pic.appendChild(img);
                }
                
                if (this.captions) {
                        var caption = document.createElement("DIV");
                        caption.className = "CaptionBox";
                        caption.id = this.rootId + "_CaptionBox";
                        caption.innerHTML = this.captions[0];
                        slideshow.appendChild(caption);
                }

                this.imageWidth = this.width;
                pic.style.width = this.width + "px";

                if (this.captions) {
                        this.imageHeight = (this.height
                                            - 32
                                            - 64 - 12);
                        pic.style.height = (this.height
                                            - 32
                                            - 64 - 12) + "px";
                } else {
                        this.imageHeight = (this.height
                                            - 32
                                            - 12);
                        pic.style.height = (this.height
                                            - 32
                                            - 12) + "px";
                }

                if (this.slides.length > 1) {
                        this.ctrl = document.createElement("DIV");
                        this.ctrl.className = "ControlBox";
                        this.ctrl.setAttribute("id", "ControlBox");
                        slideshow.appendChild(this.ctrl);
                        this.insertSlideNumbers();
                }
                
                return 0;
        }
        
        this.createShow(rootId);
        this.preloadSlides();
}




/****************************************************************/

function Photostream(rootId, photos, pathPrefix, pathPostfix, width, height)
{
        var self = this;
        this.rootId = rootId;
        this.pathPrefix = pathPrefix;
        this.pathPostfix = pathPostfix;
        this.photos = photos;
        this.self = this;
        this.curPhoto = 0;
        this.preLoad = [];
        this.photo = null;
        this.photoSelectors = [];
        this.width = width;
        this.height = height;

        this.nextPhoto = function() {
                if (self) self.selectPhoto(self.curPhoto + 1);
                else this.selectPhoto(this.curPhoto + 1);
        }

        this.prevPhoto = function() {
                if (self) self.selectPhoto(self.curPhoto - 1);
                else this.selectPhoto(this.curPhoto - 1);
        }

        this.selectPhoto = function(i) {
                this.curPhoto = i;
                if (this.curPhoto < 0) 
                        this.curPhoto = 0;
                if (this.curPhoto >= this.photos.length) 
                        this.curPhoto = this.photos.length - 1;
                this.doTransition();
        }

        this._doTransition = function() {
                self.doTransition();
        }

        this.doTransition = function() {

                if ((this.curPhoto < 0) 
                    || (this.curPhoto >= this.photos.length)) {
                        // this.photo.src = "... white ...";     FIXME
                        return;
                }
                
                var img = this.preLoad[this.curPhoto];
                if (!img) {
                        img = new Image();
                        this.preLoad[this.curPhoto] = img;
                        img.src = (this.pathPrefix + "/" 
                                   + this.photos[this.curPhoto].path 
                                   + this.pathPostfix);
                        setEventHandler(img, "load", this._doTransition);
                        return;
                }
 
                var w = img.width;
                var h = img.height;

                /* alert(img.width); */
                /* alert(img.height); */

                var tw = this.imageWidth;
                var th = this.imageHeight;
                
                if (w > tw) {
                        h = h * tw / w;
                        w = tw;
                }
                if (h > th) {
                        w = w * th / h;
                        h = th;
                }
                this.photo.width = w;
                this.photo.height = h;
                this.photo.style.top = "" + ((th - h) / 2) + "px";
                this.photo.style.left = "" + ((tw - w) / 2) + "px";
                this.photo.src = this.preLoad[this.curPhoto].src;
                this.displayDate();
        }

        this.insertLink = function(p, className, anchor, id, handler) {
                var a = document.createElement("A");
                a.className = className;
                a.id = id;
                a.setAttribute("href", "javascript:void(0)");
                a.appendChild(document.createTextNode(anchor));
                p.appendChild(a);        
                a.onclick = function() { return false; }
                a.onmousedown = function() { return false; }
                setEventHandler(a, "click", handler);
                return a;
        }

        this.displayDate = function() {
                this.dateBox.innerHTML = this.photos[this.curPhoto].datetime;
        }

        this.insertPhotoTimeline = function() {
                this.insertLink(this.ctrl, "Arrow", "<", 
                                this.rootId + "_prevPhoto", 
                                this.prevPhoto);
                                
                this.insertLink(this.ctrl, "Arrow", ">", 
                                this.rootId + "_nextPhoto", this.nextPhoto);
        }

        this.setPhotos = function(photos) {
                this.preLoad = [];
                this.curPhoto = 0;
                this.photos = photos; 
                this.doTransition();
        }

        this.createShow = function(rootId) {
                var root = document.getElementById(rootId);
                if (!root) return -1;
                root.style.width = this.width + "px";
                root.style.height = this.height + "px";
                
                var pic = document.createElement("DIV");
                pic.className = "PictureBox";
                pic.id = this.rootId + "_PictureBox";
                root.appendChild(pic);

                var img = document.createElement("IMG");
                img.className = "SlideshowImage";
                pic.appendChild(img);
                this.photo = img;
                
                this.dateBox = document.createElement("DIV");
                this.dateBox.className = "DateBox";
                this.dateBox.id = this.rootId + "_CaptionBox";
                root.appendChild(this.dateBox);

                this.imageWidth = this.width;
                this.imageHeight = (this.height
                                    - 32
                                    - 32 - 12);
                
                pic.style.width = this.width + "px";
                pic.style.height = (this.height
                                    - 32
                                    - 32 - 12) + "px";

                this.ctrl = document.createElement("DIV");
                this.ctrl.className = "ControlBox";
                this.ctrl.setAttribute("id", "ControlBox");
                root.appendChild(this.ctrl);

                this.doTransition();

                this.insertPhotoTimeline();
                
                return 0;
        }
        
        this.createShow(rootId);
}

