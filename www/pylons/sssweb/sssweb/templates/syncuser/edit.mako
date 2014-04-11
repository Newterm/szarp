<%inherit file="/admin.mako"/>

${h.form(h.url.current(method='post'))}
  <p>User name: ${h.text('name', value=c.user['name'], disabled = (len(c.user['name']) > 0))}</p>
<p>E-mail ${h.text('email', value=c.user['email'])}</p>
<p>Expire:
<p>${h.radio('exp','never', label = 'Never', checked = (c.user['expired'] == '-1'),  onchange="document.getElementById('expired').disabled = true")}</p>
<p>${h.radio('exp','date', label = 'Date', checked = (c.user['expired'] != '-1'), onchange="document.getElementById('expired').disabled = false")} 
<%
import datetime
if c.user['expired'] == '-1':
	expdate = str(datetime.datetime.now().year + 1) + "0101"
else:
	expdate = c.user['expired']
%>
${h.text('expired', value = expdate, maxlength = 8, disabled = (c.user['expired'] == '-1'))}</p>
<p>
${h.hidden('hwkey', value=c.user['hwkey'])}
% if c.user['hwkey'] == '':
Hwkey ${h.text('hwkey_text', value = 'Turned off', disabled = True)}</p>
${h.link_to('Enable key', url = h.url.current(action = 'reset_key', id = c.user['name']))}
${h.link_to('Disable key', url = h.url.current(action = 'disable_key', id = c.user['name']))}
% elif c.user['hwkey'] == '0':
Hwkey ${h.text('hwkey_text', value = 'Disabled', disabled = True)}</p>
${h.link_to('Enable key', url = h.url.current(action = 'reset_key', id = c.user['name']))}
${h.link_to('Turn off key', url = h.url.current(action = 'turnoff_key', id = c.user['name']))}
% elif c.user['hwkey'] == '-1':
Hwkey ${h.text('hwkey_text', value = 'Waiting', disabled = True)}</p>
${h.link_to('Disable key', url = h.url.current(action = 'disable_key', id = c.user['name']))}
${h.link_to('Turn off key', url = h.url.current(action = 'turnoff_key', id = c.user['name']))}
% else:
Hwkey ${h.text('hwkey_text', value= c.user['hwkey'], disabled = True)}</p>
${h.link_to('Disable key', url = h.url.current(action = 'disable_key', id = c.user['name']))}
${h.link_to('Reset key', url = h.url.current(action = 'reset_key', id = c.user['name']))}
${h.link_to('Turn off key', url = h.url.current(action = 'turnoff_key', id = c.user['name']))}
% endif
</p>
<p>Comment ${h.text('comment', value = c.user['comment'])}</p>
<p>Server ${h.select('server', c.user['server'], c.servers)}</p>
<div class="u_bases">
<%
  table = h.distribute(c.all_bases, 5, "V")
%>
<table class="bases">
% for row in table:
  <tr>
% for cell in row:
    <td>
%   if cell is not None:
      ${h.checkbox(cell, checked = (cell in c.user['sync']), label = cell)}
%   endif
    </td>
% endfor   cell
  </tr>
% endfor   row
</table>
    
</div>
${h.submit('submit', 'Save changes')}
${h.end_form()}

% if len(c.user['name']) > 0:
${h.form(h.url.current(action='reset_password', id=c.user['name'], method='post'))}
${h.submit('reset', 'Reset password')}
${h.end_form()}

${h.form(h.url.current(action='delete', id=c.user['name'], method='post'))}
${h.submit('remove', 'Remove user')}
${h.end_form()}
% endif


