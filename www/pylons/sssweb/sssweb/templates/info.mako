<%inherit file="base.mako"/>

<div class="info">
% if c.message:
  <p>${c.message}</p>
% endif
  ${h.form(url(action=c.action,controller=c.controller), method='get')}
  ${h.submit('submit', _('Ok'))}
  ${h.end_form()}
</div>

