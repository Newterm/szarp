<%inherit file="/base.mako"/>

<div class="links">
<%
    ctrl = request.environ['pylons.routes_dict']['controller']
    index = request.environ['pylons.routes_dict']['action'] == 'index'
%>
    <span>${h.link_to_unless(ctrl == 'syncuser' and index, 'Users', url(controller='syncuser', action='index'))}</span>
    <span>${h.link_to_unless(ctrl == 'syncserver' and index, 'Servers', h.url(controller='syncserver', action='index'))}</span>
    <span>${h.link_to_unless(ctrl == 'remarks' and index, 'Remarks', h.url(controller='remarks', action='index'))}</span>
    <span>${h.link_to('Logout', url('logout'))}</span>
</div>
${next.body()}

