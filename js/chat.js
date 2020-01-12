
$('#textBoxInput').on('keypress',function(e) {
    if(e.which == 13) {
        $.ajax({
            type: 'POST',
            dataType: "html",
            url: '/postMsg',
            data: {message: $('#textBoxInput').val(), idTo: $('.active').attr('data-chat')},
            success: function(data)
                {
                    if(data == "succes"){
                        getNewMessages();
                    }
                    else{
                        alert("Could not send messege");
                    }
                }
            });
            $('#textBoxInput').val('');
    }
});

$('.left .person').mousedown(function(){
    $('#textBoxInput').val('');
    if ($(this).hasClass('.active')) {
        return false;
    } else {
        var findChat = $(this).attr('data-chat');
        var personName = $(this).find('.name').text();
        $('.right .top .name').html(personName);
        $('.chat').removeClass('active-chat');
        $('.left .person').removeClass('active');
        $(this).addClass('active');
        $('.chat[data-chat = '+findChat+']').addClass('active-chat');
    }
});