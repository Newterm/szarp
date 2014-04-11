<%inherit file="base.mako"/>

<div class="links">
    <span>${h.link_to('Synchronizer', url(controller='syncuser', action='index'))}</span>
    <span>${h.link_to('Remarks', url(controller='remarks', action='index'))}</span>
    <span>${h.link_to('Logout', url(controller='login', action='logout'))}</span>
</div>

${h.form(h.url.current(method='post'))}
  <p>Login: ${h.text('name', value=c.user.name, disabled = (len(c.user.name) > 0))}</p>
  <p>User name: ${h.text('real_name', value=c.user.real_name)}</p>
  <p>Password: ${h.password('password', value=c.password)}</p>
<div class="r_bases">
<%
  table = h.distribute(c.bases_q, 5, "V")
%>
<table class="r_bases">
% for row in table:
  <tr>
% for cell in row:
    <td>
%   if cell is not None:
      ${h.checkbox(cell[0], checked = (cell[0] in c.user_ids), label = cell[1])}
%   endif
    </td>
% endfor   cell
  </tr>
% endfor   row
</table>
    
</div>
${h.submit('submit', 'Save changes')}
${h.end_form()}

