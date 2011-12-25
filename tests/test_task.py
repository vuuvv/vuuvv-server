import sys
import unittest
from pake import task, tasks, loader

result = []
loader.module = sys.modules[__name__]

@task()
def all(depends, target):
	pass

@task()
def a(depends, target):
	result.append('a')

@task([], target="iamb")
def b(depends, target):
	result.append('b')

@task([a,b])
def c(depends, target):
	result.append('c')

@task([b, c], default=True)
def d(depends, target):
	result.append('d')

class TestTask(unittest.TestCase):
	def setUp(self):
		result = []

	def test_name(self):
		self.assertEqual('a', a)
		self.assertEqual('iamb', b)
		self.assertEqual('c', c)
		self.assertEqual('d', d)
		self.assertEqual('all', all)

	def test_flow(self):
		tasks.run('d')
		self.assertEqual(['b', 'a', 'c', 'd'], result)

	def test_find(self):
		module = sys.modules[__name__]
		self.assertEqual([], tasks.find(module, 'a')['depends'])
		self.assertEqual([], tasks.find(module, 'iamb')['depends'])
		self.assertEqual([a,b], tasks.find(module, 'c')['depends'])
		self.assertEqual([b,c], tasks.find(module, 'd')['depends'])
		self.assertEqual([b,c], tasks.find(module)['depends'])

