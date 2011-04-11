<%inherit file="/base.mako"/>

<div class="links">
<%
    action = request.environ['pylons.routes_dict']['action']
%>
    ${_('Welcome')} ${session['user']}!
    ${h.link_to(_('Logout'), h.url_for(controller='login', action='logout'))}
</div>
${next.body()}

<!-- vim:set ft=mako: -->
