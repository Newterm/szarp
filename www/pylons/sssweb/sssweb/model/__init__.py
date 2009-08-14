import sqlalchemy as sa
import sqlalchemy.orm as orm
from sqlalchemy import schema, types
from sssweb.model import meta
from pylons import config

class Prefix(object):
	pass

class Users(object):
	pass

class UsersPrefixAssociation(object):
	pass

def init_model(engine):
	"""Call me before using any of the tables or classes in the model"""
	global prefix_table
	prefix_table = schema.Table('prefix', meta.metadata, 
			schema.Column('id', types.Integer,
				schema.Sequence('prefix_id_seq'), primary_key=True),
			schema.Column('prefix', types.Text())
			)
	
	global users_table
	users_table = schema.Table('users', meta.metadata, 
			schema.Column('id', types.Integer,
				schema.Sequence('users_id_seq'), primary_key=True),
			schema.Column('name', types.Text()),
			schema.Column('password', types.Text()),
			schema.Column('real_name', types.Text())
			)
	
	global users_prefix_access
	users_prefix_access = schema.Table('user_prefix_access', meta.metadata, 
			schema.Column('user_id', types.Integer, schema.ForeignKey('users.id'), primary_key=True),
			schema.Column('prefix_id', types.Integer, schema.ForeignKey('prefix.id'), primary_key=True),
			schema.Column('write_access', types.Integer)
			)

	orm.mapper(Users, users_table, properties = { 'base':orm.relation(UsersPrefixAssociation) })
	orm.mapper(UsersPrefixAssociation, users_prefix_access, properties = { 'prefix':orm.relation(Prefix) })
	orm.mapper(Prefix, prefix_table)
	
	meta.Session.configure(bind=engine)
	meta.engine = engine

