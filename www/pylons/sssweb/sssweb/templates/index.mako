<%inherit file="admin.mako"/>

<p>
<a href="${h.url_for(action='add_user')}">Add new user</a>
<p>

<table>
  <thead><th>Login</th><th>E-mail</th><th>Serwer</th><th>Expired</th><th>Key</th><th>Comment</th></thead>
% for user in c.list:
    <tr>
      <td>
        <a href="${h.url_for(action='user', id=user['name'])}">${user['name']}</a></td>
      <td>${user['email']}</td>
      <td>${user['server']}</td>
      <td>${printexp(user['expired'])}</td>
      <td>${printkey(user['hwkey'])}</td>
      <td>${user['comment']}</td>
    </tr>
% endfor
</table>

<%def name="printkey(key)">
  % if key == '0':
	Disabled
  % elif key == '':
	Turned off
  % elif key == '-1':
	Waiting
  % else:
	${key}
  % endif
</%def>

<%def name="printexp(exp)">
  % if exp == "-1":
	Never
  % else:
	${exp[0:4] + "-" + exp[4:6] + "-" + exp[6:8]}
  % endif
</%def>
