var ISL = {
    request_sent : false,
    ajax_settings :  {
        type: 'POST',
        url: location.href + '?type=defs', // gets http address and adds ?type=defs
        data: 'whatever=send',
        dataType: 'json',
        timeout: 25000,
    },
};

function ISL_ajaxFail (jqXHR, textStatus, errorThrown) {
    console.log('ajaxFail: ' + textStatus);
}

function ISL_alertContents (data, textStatus, jqXHR) {
    console.log('done: ' + textStatus);
    ISL.request_sent = false;

    for (var eid in data) {
        var val = data[eid];
        var e = document.getElementById(eid);

        if (typeof val == 'string') {
            $(e).text(val);
        }
        else { // if not string then object
            for (var attrname in val) {
                var v = val[attrname];
                if (v < 0 && ['height', 'width'].indexOf(attrname) >= 0) {
                    v = 0;
                }
                try {
                    $(e).attr(attrname, v);
                }
                catch (e) {
                    console.log('ISL: updating document error: ' + e);
                }
            }
        }
    }
}

function ISL_getRemoteData () {
        if (ISL.request_sent) {
            console.log('ISL: request pending, not sending another');
            return;
        }

        $.ajax(ISL.ajax_settings)
            .done(ISL_alertContents)
            .fail(ISL_ajaxFail)
            .always(function () {
               console.log('ajax always');
               ISL.request_sent = false;
            });

        ISL.request_sent = true;

}



//refresh time [ms]
var refresh_time = 15000;

function isl_start(evt) {
    ISL.refresh_interval = setInterval(ISL_getRemoteData, refresh_time);
}


$(document).ready(isl_start, false );

