<%inherit file="/base.mako"/>

<div class="links">
    ${_('Welcome')} ${session['user']}!
    ${h.link_to(_('Logout'), h.url('logout'))}
</div>
${next.body()}

<!-- vim:set ft=mako: -->
