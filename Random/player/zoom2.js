// Get a handle to the player
container    = jQuery('.video-container');  
player       = jQuery('.video-element')[0];
btnPlay      = jQuery('.play');
btnPause     = jQuery('.pause');
btnStop      = jQuery('.stop');
btnReplay    = jQuery('.replay');
btnMute      = jQuery('.mute');
btnUnmute    = jQuery('.unmute');
btnZoomIn    = jQuery('.zoomin');
btnZoomOut   = jQuery('.zoomout');
progressBar  = jQuery('.progress');
volumeBar    = jQuery('.volume');
btnFull      = jQuery('.fullscreen');

// Update the video volume


player.height = container.height();
player.width = container.width();
var zoom = 1.0;

volumeBar.change(function(){player.volume = volumeBar.val();});
player.addEventListener('play', function() {
        btnPlay.hide();
        btnPause.show();
    }, false);
  
player.addEventListener('pause', function() {
        btnPlay.show();
        btnPause.hide();
    }, false);

player.addEventListener('ended', function() { this.pause(); }, false);

var progressBarUpdate = true;
window.setInterval(function(){
        if(progressBarUpdate)
            progressBar.val(1000.0*player.currentTime/player.duration);
    }, 100);
  
progressBar.click(function(e){
        player.currentTime = player.duration * progressBar.val()/1000;
    });

progressBar.change(function(e){
        player.currentTime = player.duration * progressBar.val()/1000;
    });

progressBar.mousedown(function(e){
        progressBarUpdate = false;
    });
progressBar.mouseup(function(e){
        progressBarUpdate = true;
    });

btnPlay.click(function(){
        btnPlay.hide();
        btnPause.show();
        player.play();
    });
btnPause.click(function(){
        btnPlay.show();
        btnPause.hide();
        player.pause();
    });
btnStop.click(function(){
        player.pause();
        if (player.currentTime) player.currentTime = 0;
        btnPlay.show();
        btnPause.hide();
    });
// Replays the media currently loaded in the player
btnReplay.click(function(){
        btnPlay.hide();
        btnPause.show();
        player.currentTime = 0;
        player.play();
    });
// Toggles the media player's mute and unmute status
btnMute.click(function(){
        player.muted = true;
        btnMute.hide();
        btnUnmute.show();
    });
btnUnmute.click(function(){
        player.muted = false;
        btnMute.show();
        btnUnmute.hide();
    });
btnZoomIn.click(function(){
        
        zoom = Math.min(zoom*1.05, 100);
        player.height = zoom*container.height();
        player.width = zoom*container.width();
    });

btnZoomOut.click(function(){
        zoom = Math.max(zoom/1.05, 1);
        player.height = zoom*container.height();
        player.width = zoom*container.width();
    });
// Update the progress bar
function updateProgressBar() {
    // Work out how much of the media has played via the duration and currentTime parameters
    var percentage = Math.floor((100 / player.duration) * player.currentTime);
    // Update the progress bar's value
    progressBar.value = percentage;
    // Update the progress bar's text (for browsers that don't support the progress element)
    progressBar.innerHTML = percentage + '% played';
}
  
function resetPlayer() {
    // progressBar.value = 0;
    // Move the media back to the start
    player.currentTime = 0;
    // Set the play/pause button to 'play'
    ntnPlayPause.removeClass('pause');
}  
  
function exitFullScreen() {
    if (document.exitFullscreen) {
        document.exitFullscreen();
    } else if (document.msExitFullscreen) {
        document.msExitFullscreen();
    } else if (document.mozCancelFullScreen) {
        document.mozCancelFullScreen();
    } else if (document.webkitExitFullscreen) {
        document.webkitExitFullscreen();
    }
}
  
btnFull.click(function(){
        if (player.requestFullscreen)
            if (document.fullScreenElement) {
                document.cancelFullScreen();
            } else {
                player.requestFullscreen();
            }
        else if (player.msRequestFullscreen)
            if (document.msFullscreenElement) {
                document.msExitFullscreen();
            } else {
                player.msRequestFullscreen();
            }
        else if (player.mozRequestFullScreen)
            if (document.mozFullScreenElement) {
                document.mozCancelFullScreen();
            } else {
                player.mozRequestFullScreen();
            }
        else if (player.webkitRequestFullscreen)
            if (document.webkitFullscreenElement) {
                document.webkitCancelFullScreen();
            } else {
                player.webkitRequestFullscreen();
            }
        else {
            alert("Fullscreen API is not supported");       
        }
    });