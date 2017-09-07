    jQuery(function(){
       var counter=0;
       var parseHTMLtree = function(element) {
            var tree = {};     
            tree.name = element.prop('tagName');
            var id = element.attr('id');
            if(id) tree.name=tree.name+'#'+id; else id='_c'+counter++;
            element.attr('id',id);
            element.attr('old-color',element.css('background-color'));
            var classes = element.attr('class');
	    if(classes) tree.name = tree.name+'.'+classes.replace(/[ ]+/g,'.');
            tree.children = [];
	    tree.html = '<span'+(id?(' rel="'+id+'">'):'>')+tree.name+'</span><ul style="list-style:none;">';
            element.children().each(function(i){
                    var node = parseHTMLtree(jQuery(this));
		    tree.children.push(node);
                    tree.html = tree.html + '<li>'+node.html+'</li>';
		});
	    if(!tree.children.length) delete tree['children'];
            tree.html = tree.html + '</ul>';
	    return tree;
       };
       var tree = parseHTMLtree(jQuery('body'));
       var html = JSON.stringify(tree);
       jQuery('body').append('<div id="bwidget" style="z-index:1000;position:fixed;right:0;bottom:0;border:1px solid black;font-size:10px;font-family:helvetica;max-height:600px;overflow-y: scroll;">'+tree.html+'<input name="selector" style="width:100%"/></div>');
       jQuery('[rel]').each(function(i){
	       var j=jQuery(this);
               var k=jQuery('#'+j.attr('rel'));
               k.mouseover(function(){j.css('background-color','yellow');});
	       k.mouseout(function(){j.css('background-color','transparent');});
               j.mouseover(function(){k.css('background-color','yellow');j.css('background-color','yellow');});
	       j.mouseout(function(){k.css('background-color',k.attr('old-color'));j.css('background-color','transparent');});
	   });
       var sel = jQuery('input[name=selector]');
       sel.keyup(function(){jQuery('[bchange]').each(function(i){jQuery(this).css('background-color',jQuery(this).attr('old-color')).attr('bchange',false);});jQuery(sel.val()).css('background-color','yellow').attr('bchange',true);jQuery('#bwidget '+sel.val()).css('background-color','transparent');});
    });

   

/*
var urls = ['http://www.geonames.org/postalCodeLookupJSON?postalcode=60302&country=US&callback=?','http://services.digg.com/stories/top?appkey=http%3A%2F%2Fmashup.com&type=javascript&callback=?','http://api.flickr.com/services/feeds/photos_public.gne?tags=cat&tagmode=any&format=json&jsoncallback=?','http://local.yahooapis.com/LocalSearchService/V3/localSearch?appid=YahooDemo&query=pizza&zip=10504&results=2&output=json&callback=?'];
      for(var i=0; i<urls.length; i++)
         jQuery.getJSON(urls[i],function(data) {jQuery('body').append(jQuery('<div/>').text(JSON.stringify(data)).html());});
*/