<%inherit file="base.mako"/>

<div class="links">
    <span>${h.link_to('Synchronizer', h.url(controller='syncuser', action='index'))}</span>
    <span>${h.link_to('Logout', h.url(controller='login', action='logout'))}</span>
</div>

<p>${h.link_to('Add user', h.url.current(action = 'new'))}</p>

<table>
  <thead>
    <th>Login:</th>
    <th>Users:</th>
</thead>
% for p in c.users_q:
      <tr>
            <td><b>${h.link_to(p.name, url = h.url.current(action = 'edit', id = p.id))}</b></td>
            <td><b>${h.link_to(p.real_name, url = h.url.current(action = 'edit', id = p.id))}</b></td>
            <td>${h.link_to('Delete', url = h.url.current(action='delete', id=p.id))}</td>
    </tr> 
% endfor
</table>
